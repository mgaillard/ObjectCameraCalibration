import os
os.environ["CUDA_VISIBLE_DEVICES"] = "1"

import datetime
import math
from time import time


import tensorflow as tf
from tensorflow import keras

from tensorflow.keras.models import Model, Sequential, load_model
from tensorflow.keras.layers import Dense, Dropout, Flatten, Input, Conv2D, MaxPooling2D, Lambda, Concatenate
from tensorflow.keras.callbacks import TensorBoard, LearningRateScheduler

import numpy as np

# Learning parameters
img_rows, img_cols = 302, 403
input_shape = (img_rows, img_cols, 4)
batch_size = 64
epochs = 40
model_load = False
model_train = True
model_save = True
model_predict = True
model_file = 'calibration_model.h5'
dataset_folder = "E:\\CS635\\quaternion_dataset"
# dataset_folder = "/mnt/e/CS635/dataset"
test_folder = os.path.join(dataset_folder, "test")
train_folder = os.path.join(dataset_folder, "train")
validation_folder = os.path.join(dataset_folder, "validation")

translation_range = 0.2
rotation_range = 45.0
rotation_z_range = 180.0
quaternion_range = 1.0

preprocessed_translation_xy_range = translation_range / (1.0 - translation_range)
preprocessed_translation_z_range = 1.0 + translation_range

def transform_to_world(translations):
    # Reverse preprocessing
    translations[0] = translations[0] * preprocessed_translation_xy_range
    translations[1] = translations[1] * preprocessed_translation_xy_range
    translations[2] = translations[2] * preprocessed_translation_z_range
    # X = x*z and Y = y*z
    translations[0] = translations[0] * translations[2]
    translations[1] = translations[1] * translations[2]
    # Z = 1 - z
    translations[2] = 1.0 - translations[2]
    return translations

def compute_translation_error(y_true, y_pred):
    translations_true = tf.gather(y_true, [0, 1, 2], axis=1)
    translations_pred = tf.gather(y_pred, [0, 1, 2], axis=1)
    
    # Reverse preprocessing
    translations_true = tf.math.multiply(translations_true, [preprocessed_translation_xy_range,
                                                             preprocessed_translation_xy_range,
                                                             preprocessed_translation_z_range])

    translations_pred = tf.math.multiply(translations_pred, [preprocessed_translation_xy_range,
                                                             preprocessed_translation_xy_range,
                                                             preprocessed_translation_z_range])

    # Extract z coordinate
    translations_true_z = tf.gather(translations_true, [2], axis=1)
    translations_pred_z = tf.gather(translations_pred, [2], axis=1)

    # Extract x and y coordinate
    translations_true_xy = tf.gather(translations_true, [0, 1], axis=1)
    translations_pred_xy = tf.gather(translations_pred, [0, 1], axis=1)
    
    # X = x*z and Y = y*z
    translations_true_xy = tf.math.multiply(translations_true_xy, translations_true_z)
    translations_pred_xy = tf.math.multiply(translations_pred_xy, translations_pred_z)
    
    # Z = 1 - z
    translations_true_z = tf.math.subtract(translations_true_z, [1.0])
    translations_true_z = tf.math.multiply(translations_true_z, [-1.0])
    translations_pred_z = tf.math.subtract(translations_pred_z, [1.0])
    translations_pred_z = tf.math.multiply(translations_pred_z, [-1.0])
    
    # Concat XY and Z
    translations_true = tf.concat([translations_true_xy, translations_true_z], 1)
    translations_pred = tf.concat([translations_pred_xy, translations_pred_z], 1)

    return translations_true - translations_pred

def compute_rotation_error(y_true, y_pred):
    rotations_true = tf.gather(y_true, [3, 4, 5], axis=1)
    rotations_pred = tf.gather(y_pred, [3, 4, 5], axis=1)
    return tf.math.multiply(rotations_true - rotations_pred, [rotation_range, rotation_range, rotation_z_range])

# Infinity norm over the dataset
def tran_max(y_true, y_pred):
    translationErrors = compute_translation_error(y_true, y_pred)
    return tf.norm(translationErrors, ord=np.inf)

# Infinity norm over the dataset
def rot_max(y_true, y_pred):
    rotationErrors = compute_rotation_error(y_true, y_pred)
    return tf.norm(rotationErrors, ord=np.inf)

# Average norm over the dataset
def tran_avg(y_true, y_pred):
    translationErrors = compute_translation_error(y_true, y_pred)
    return tf.reduce_mean(tf.math.abs(translationErrors))

# Average norm over the dataset
def rot_avg(y_true, y_pred):
    rotationErrors = compute_rotation_error(y_true, y_pred)
    return tf.reduce_mean(tf.math.abs(rotationErrors))

def quaternion_angles(quaternion_true, quaternion_pred):
    # Normalize
    unit_quaternion_true = tf.math.l2_normalize(quaternion_true, axis=-1)
    unit_quaternion_pred = tf.math.l2_normalize(quaternion_pred, axis=-1)
    # Dot product between the 2 unit quaternions
    abs_dot_product = tf.math.abs(tf.reduce_sum(tf.multiply(unit_quaternion_true, unit_quaternion_pred), axis=-1))
    # Estimate angle
    return 2.0 * tf.math.acos(tf.clip_by_value(abs_dot_product, clip_value_min=0.0, clip_value_max=1.0))

def quat_max(y_true, y_pred):
    # Extract quaternion
    quaternion_true = tf.gather(y_true, [3, 4, 5, 6], axis=1)
    quaternion_pred = tf.gather(y_pred, [3, 4, 5, 6], axis=1)
    return tf.math.reduce_max(quaternion_angles(quaternion_true, quaternion_pred))

def quat_avg(y_true, y_pred):
    # Extract quaternion
    quaternion_true = tf.gather(y_true, [3, 4, 5, 6], axis=1)
    quaternion_pred = tf.gather(y_pred, [3, 4, 5, 6], axis=1)
    return tf.reduce_mean(quaternion_angles(quaternion_true, quaternion_pred))

def pose_loss(y_true, y_pred):
    translations_true = tf.gather(y_true, [0, 1, 2], axis=1)
    translations_pred = tf.gather(y_pred, [0, 1, 2], axis=1)
    quaternion_true = tf.gather(y_true, [3, 4, 5, 6], axis=1)
    quaternion_pred = tf.gather(y_pred, [3, 4, 5, 6], axis=1)
    # Sub loss for translations
    translation_loss = keras.losses.mean_squared_error(translations_true, translations_pred)
    # Sub loss for quaternions
    quaternion_loss = quaternion_angles(quaternion_true, quaternion_pred) / math.pi
    # Sub loss to make sure the quaternion are unit
    quaternion_norm = tf.math.square(tf.norm(quaternion_pred, ord=2) - 1.0)
    return translation_loss + quaternion_loss + 0.2 * quaternion_norm

# Load and preprocess an image in the dataset
def read_image(file_path):
    # Read the PNG image file
    img = tf.io.read_file(file_path)
    img = tf.image.decode_png(img, channels=4)
    # img = tf.image.resize(img, [img_cols, img_rows])
    img = tf.image.convert_image_dtype(img, tf.float32)
    
    # Open the txt file and read the parameters
    txt_file_path = tf.strings.regex_replace(file_path, "\.png$", ".txt")
    txt_file = tf.io.read_file(txt_file_path)
    parameters = tf.strings.to_number(tf.strings.split(txt_file, sep='\n'))

    # Compute depth: z = 1 - z
    parameters = tf.math.subtract(parameters, [0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0])
    parameters = tf.math.multiply(parameters, [1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 1.0])
    # Isolate the depth value
    z = tf.math.reduce_sum(tf.gather(parameters, [2]))
    # Divide X and Y by the depth
    parameters = tf.math.divide(parameters, [z, z, 1.0, 1.0, 1.0, 1.0, 1.0])

    # Normalize data
    parameters = tf.math.divide(parameters, [preprocessed_translation_xy_range,
                                             preprocessed_translation_xy_range,
                                             preprocessed_translation_z_range,
                                             quaternion_range,
                                             quaternion_range,
                                             quaternion_range,
                                             quaternion_range])
    
    return img, parameters

# Load training set
train_dataset = tf.data.Dataset.list_files(os.path.join(train_folder, '*.png'), shuffle=False) \
                               .map(read_image, num_parallel_calls=tf.data.experimental.AUTOTUNE) \
                               .shuffle(8000) \
                               .batch(batch_size) \
                               .prefetch(tf.data.experimental.AUTOTUNE)

# Load validation set
validation_dataset = tf.data.Dataset.list_files(os.path.join(validation_folder, '*.png'), shuffle=False) \
                                    .map(read_image, num_parallel_calls=tf.data.experimental.AUTOTUNE).cache() \
                                    .batch(batch_size) \
                                    .prefetch(tf.data.experimental.AUTOTUNE)

if model_load:
    # Just load the model without training
    model = load_model(model_file, custom_objects={
        'tran_max': tran_max,
        'rot_max': rot_max,
        'tran_avg': tran_avg,
        'rot_avg': rot_avg,
        'quat_avg': quat_avg,
        'quat_max': quat_max})
    model.summary()

else:
    # Create the model with multi GPU support
    # strategy = tf.distribute.MirroredStrategy(cross_device_ops=tf.distribute.HierarchicalCopyAllReduce())
    # with strategy.scope():
    # Define the tensors for the two input images
    input = Input(shape=input_shape)
    x = Conv2D(4, (3, 3), activation='relu', padding='same')(input)
    x = Conv2D(4, (3, 3), activation='relu', padding='same')(x)
    x = MaxPooling2D((2, 2), strides=(2, 2))(x)
    x = Conv2D(4, (3, 3), activation='relu', padding='same')(x)
    x = Conv2D(4, (3, 3), activation='relu', padding='same')(x)
    x = MaxPooling2D((2, 2), strides=(2, 2))(x)
    x = Conv2D(8, (3, 3), activation='relu', padding='same')(x)
    x = Conv2D(8, (3, 3), activation='relu', padding='same')(x)
    x = MaxPooling2D((2, 2), strides=(2, 2))(x)
    x = Conv2D(8, (3, 3), activation='relu', padding='same')(x)
    x = Conv2D(8, (3, 3), activation='relu', padding='same')(x)
    x = Flatten()(x)
    x = Dense(256, activation='relu')(x)
    x = Dropout(0.05)(x)
    x = Dense(256, activation='relu')(x)
    x = Dropout(0.05)(x)
    prediction = Dense(7, activation='linear')(x)
    model = Model(inputs=input, outputs=prediction)
    model.compile(loss='mean_squared_error', optimizer='adam', metrics=[tran_avg, tran_max, quat_avg, quat_max])

    # Display model summary
    model.summary()

if model_train:
    # Setup tensorboard
    log_dir = "logs\\fit\\" + datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    file_writer = tf.summary.create_file_writer(log_dir + "\\images")
    tensorboard_callback = TensorBoard(log_dir=log_dir, histogram_freq=1)

    model.fit(train_dataset,
              validation_data=validation_dataset,
              epochs=epochs,
              callbacks=[tensorboard_callback])

if model_save:
    # Save the model
    model.save(model_file)

if model_predict:
    test_dataset = tf.data.Dataset.list_files(os.path.join(test_folder, '*.png'), shuffle=False) \
                                  .map(read_image, num_parallel_calls=tf.data.experimental.AUTOTUNE) \
                                  .batch(batch_size) \
                                  .prefetch(tf.data.experimental.AUTOTUNE)

    predictions = model.predict(test_dataset, verbose=1)

    for i in range(0, len(predictions)):
        translations = transform_to_world(predictions[i][0:3])
        rotations = np.multiply(predictions[i][3:7], [quaternion_range, quaternion_range, quaternion_range, quaternion_range])
        rotations = rotations / np.linalg.norm(rotations, 2)
        filepath = os.path.join(test_folder, "../test_full", "{:05d}_pred.txt".format(i))
        with open(filepath, "w") as file:
            file.write("{:.2f}\n".format(translations[0]))
            file.write("{:.2f}\n".format(translations[1]))
            file.write("{:.2f}\n".format(translations[2]))
            file.write("{:.4f}\n".format(rotations[0]))
            file.write("{:.4f}\n".format(rotations[1]))
            file.write("{:.4f}\n".format(rotations[2]))
            file.write("{:.4f}\n".format(rotations[3]))
