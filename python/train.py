import os
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
batch_size = 250
epochs = 100
model_load = True
model_train = False
model_save = False
model_file = 'calibration_model.h5'
dataset_folder = "E:\\CS635\\dataset"
testset_folder = "E:\\CS635\\testset"
train_folder = os.path.join(dataset_folder, "train")
validation_folder = os.path.join(dataset_folder, "validation")
translation_range = 0.2
rotation_range = 45.0

# This function keeps the learning rate at 0.002 for the first ten epochs.
def scheduler(epoch):
    if epoch < 10:
        return 0.002
    else:
        return 0.001

# Infinity norm averaged over the dataset
def translation_inf(y_true, y_pred):
    return translation_range * tf.norm(y_true[0:3] - y_pred[0:3], ord=np.inf)

# Infinity norm averaged over the dataset
def rotation_inf(y_true, y_pred):
    return rotation_range * tf.norm(y_true[3:6] - y_pred[3:6], ord=np.inf)

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

    # Normalize data
    parameters = tf.math.divide(parameters, [translation_range, translation_range, translation_range, rotation_range, rotation_range, rotation_range])
    
    return img, parameters

# Load training set
train_dataset = tf.data.Dataset.list_files(train_folder + '\\*.png', shuffle=False) \
                               .map(read_image, num_parallel_calls=tf.data.experimental.AUTOTUNE) \
                               .shuffle(2000) \
                               .batch(batch_size) \
                               .prefetch(tf.data.experimental.AUTOTUNE)

# Load validation set
validation_dataset = tf.data.Dataset.list_files(validation_folder + '\\*.png', shuffle=False) \
                                    .map(read_image, num_parallel_calls=tf.data.experimental.AUTOTUNE).cache() \
                                    .batch(batch_size) \
                                    .prefetch(tf.data.experimental.AUTOTUNE)

if model_load:
    # Just load the model without training
    model = load_model(model_file, custom_objects={
        'translation_inf': translation_inf,
        'rotation_inf': rotation_inf})
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
    x = Flatten()(x)
    x = Dense(256, activation='relu')(x)
    x = Dropout(0.2)(x)
    x = Dense(256, activation='relu')(x)
    x = Dropout(0.2)(x)
    prediction = Dense(6, activation='linear')(x)
    model = Model(inputs=input, outputs=prediction)
    model.compile(loss='mean_squared_error', optimizer='adam', metrics=[translation_inf, rotation_inf])

    # Display model summary
    model.summary()

if model_train:
    # Setup tensorboard
    log_dir = "logs\\fit\\" + datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    file_writer = tf.summary.create_file_writer(log_dir + "\\images")
    tensorboard_callback = TensorBoard(log_dir=log_dir, histogram_freq=1)

    # Variable learning rate
    scheduler_callback = LearningRateScheduler(scheduler)

    model.fit(train_dataset,
              validation_data=validation_dataset,
              epochs=epochs,
              callbacks=[tensorboard_callback, scheduler_callback])

if model_save:
    # Save the model
    model.save(model_file)


# Average infinity norm between prediction and ground-truth i nthe validation dataset
mean_translation_error = 0.0
mean_rotation_error = 0.0
dataset_size = 0

for images, parameters in validation_dataset:
    parameters_true = parameters.numpy()
    parameters_pred = model.predict(images, verbose=0)

    for i in range(0, len(parameters_pred)):
        translation_true = translation_range * parameters_true[i][0:3]
        translation_pred = translation_range * parameters_pred[i][0:3]
        rotation_true = rotation_range * parameters_true[i][3:6]
        rotation_pred = rotation_range * parameters_pred[i][3:6]

        mean_translation_error += np.linalg.norm(translation_true - translation_pred, np.inf)
        mean_rotation_error += np.linalg.norm(rotation_true - rotation_pred, np.inf)
        dataset_size += 1

mean_translation_error /= dataset_size
mean_rotation_error /= dataset_size
print('Number of images: ' + str(dataset_size))
print('Average infinity norm for translations: ' + str(mean_translation_error))
print('Average infinity norm for rotations: ' + str(mean_rotation_error))
