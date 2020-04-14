import os
os.environ["CUDA_VISIBLE_DEVICES"] = "1"

import datetime
from time import time

import tensorflow as tf
from tensorflow import keras

from tensorflow.keras.datasets import mnist
from tensorflow.keras.models import Model, Sequential, load_model
from tensorflow.keras.layers import Dense, Dropout, Flatten, Input, Conv2D, MaxPooling2D, Lambda, Concatenate
from tensorflow.keras.callbacks import TensorBoard, LearningRateScheduler

import numpy as np

from Dataset import Dataset
from Images import Images

# Learning parameters
img_rows, img_cols = 302, 403
input_shape = (img_rows, img_cols, 4)
batch_size = 64
epochs = 40
model_load = True
model_train = False
model_save = False
model_evaluate = False
model_predict = True
model_file = 'calibration_model.h5'
dataset_folder = "E:\\CS635\\5d_dataset"
# dataset_folder = "/mnt/e/CS635/dataset"
test_folder = os.path.join(dataset_folder, "test")
train_folder = os.path.join(dataset_folder, "train")
validation_folder = os.path.join(dataset_folder, "validation")

translation_range = 0.2
rotation_range = 45.0
rotation_z_range = 180.0

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
    parameters = tf.math.subtract(parameters, [0.0, 0.0, 1.0, 0.0, 0.0, 0.0])
    parameters = tf.math.multiply(parameters, [1.0, 1.0, -1.0, 1.0, 1.0, 1.0])
    # Isolate the depth value
    z = tf.math.reduce_sum(tf.gather(parameters, [2]))
    # Divide X and Y by the depth
    parameters = tf.math.divide(parameters, [z, z, 1.0, 1.0, 1.0, 1.0])

    # Normalize data
    parameters = tf.math.divide(parameters, [preprocessed_translation_xy_range,
                                             preprocessed_translation_xy_range,
                                             preprocessed_translation_z_range,
                                             rotation_range,
                                             rotation_range,
                                             rotation_z_range])
    
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
        'rot_avg': rot_avg})
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
    prediction = Dense(6, activation='linear')(x)
    model = Model(inputs=input, outputs=prediction)
    model.compile(loss='mean_squared_error', optimizer='adam', metrics=[tran_avg, rot_avg, tran_max, rot_max])

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

if model_evaluate:
    # Average infinity norm between prediction and ground-truth in the validation dataset
    mean_translation_error = 0.0
    mean_rotation_error = 0.0
    dataset_size = 0

    for images, parameters in validation_dataset:
        parameters_true = parameters.numpy()
        parameters_pred = model.predict(images, verbose=0)

        for i in range(0, len(parameters_pred)):
            translation_true = transform_to_world(parameters_true[i][0:3])
            translation_pred = transform_to_world(parameters_pred[i][0:3])
            rotation_true = np.multiply(parameters_true[i][3:6], [rotation_range, rotation_range, rotation_z_range])
            rotation_pred = np.multiply(parameters_pred[i][3:6], [rotation_range, rotation_range, rotation_z_range])

            mean_translation_error += np.linalg.norm(translation_true - translation_pred, np.inf)
            mean_rotation_error += np.linalg.norm(rotation_true - rotation_pred, np.inf)
            dataset_size += 1

    mean_translation_error /= dataset_size
    mean_rotation_error /= dataset_size
    print('Number of images: ' + str(dataset_size))
    print('Average infinity norm for translations: ' + str(mean_translation_error))
    print('Average infinity norm for rotations: ' + str(mean_rotation_error))

if model_predict:
    test_dataset = tf.data.Dataset.list_files(os.path.join(test_folder, '*.png'), shuffle=False) \
                                  .map(read_image, num_parallel_calls=tf.data.experimental.AUTOTUNE) \
                                  .batch(batch_size) \
                                  .prefetch(tf.data.experimental.AUTOTUNE)

    predictions = model.predict(test_dataset, verbose=1)

    for i in range(0, len(predictions)):
        translations = transform_to_world(predictions[i][0:3])
        rotations = np.multiply(predictions[i][3:6], [rotation_range, rotation_range, rotation_z_range])
        filepath = os.path.join(test_folder, "../test_full", "{:05d}.txt".format(i))
        with open(filepath, "w") as file:
            file.write("{:.2f}\n".format(translations[0]))
            file.write("{:.2f}\n".format(translations[1]))
            file.write("{:.2f}\n".format(translations[2]))
            file.write("{:.2f}\n".format(rotations[0]))
            file.write("{:.2f}\n".format(rotations[1]))
            file.write("{:.2f}\n".format(rotations[2]))
