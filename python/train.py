import os
import datetime
from time import time

import tensorflow as tf
from tensorflow import keras

from tensorflow.keras.datasets import mnist
from tensorflow.keras.models import Model, Sequential, load_model
from tensorflow.keras.layers import Dense, Dropout, Flatten, Input, Conv2D, MaxPooling2D, Lambda, Concatenate
from tensorflow.keras.callbacks import TensorBoard

import numpy as np

from Dataset import Dataset
from Images import Images

# Setup tensorboard
log_dir = "logs\\fit\\" + datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
file_writer = tf.summary.create_file_writer(log_dir + "\\images")
tensorboard_callback = TensorBoard(log_dir=log_dir, histogram_freq=1)

# Learning parameters
img_rows, img_cols = 302, 403
input_shape = (img_rows, img_cols, 4)
batch_size = 250
epochs = 12
train_model = True
model_file = 'calibration_model.h5'
dataset_folder = "E:\\CS635\\dataset\\train"
valset_folder = "E:\\CS635\\dataset\\validation"
testset_folder = "E:\\CS635\\testset"

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
    
    return img, parameters

# Load training set
train_dataset = tf.data.Dataset.list_files(dataset_folder + '\\*.png', shuffle=False) \
                               .map(read_image, num_parallel_calls=tf.data.experimental.AUTOTUNE) \
                               .shuffle(2000) \
                               .batch(batch_size) \
                               .prefetch(tf.data.experimental.AUTOTUNE)

# Load validation set
validation_dataset = tf.data.Dataset.list_files(valset_folder + '\\*.png', shuffle=False) \
                                    .map(read_image, num_parallel_calls=tf.data.experimental.AUTOTUNE).cache() \
                                    .batch(batch_size) \
                                    .prefetch(tf.data.experimental.AUTOTUNE)

if train_model:
    # Create the model with multi GPU support
    # strategy = tf.distribute.MirroredStrategy(cross_device_ops=tf.distribute.HierarchicalCopyAllReduce())
    # with strategy.scope():
    # Define the tensors for the two input images
    input = Input(shape=input_shape)
    x = Conv2D(4, (3, 3), activation='relu', padding='same')(input)
    x = Conv2D(4, (3, 3), activation='relu', padding='same')(x)
    x = MaxPooling2D((2, 2), strides=(2, 2))(x)
    x = Conv2D(8, (3, 3), activation='relu', padding='same')(x)
    x = Conv2D(8, (3, 3), activation='relu', padding='same')(x)
    x = MaxPooling2D((2, 2), strides=(2, 2))(x)
    x = Conv2D(8, (3, 3), activation='relu', padding='same')(x)
    x = Conv2D(8, (3, 3), activation='relu', padding='same')(x)
    x = MaxPooling2D((2, 2), strides=(2, 2))(x)
    x = Conv2D(16, (3, 3), activation='relu', padding='same')(x)
    x = Conv2D(16, (3, 3), activation='relu', padding='same')(x)
    x = Flatten()(x)
    x = Dense(64, activation='relu')(x)
    x = Dropout(0.2)(x)
    x = Dense(64, activation='relu')(x)
    x = Dropout(0.2)(x)
    prediction = Dense(6, activation='linear')(x)
    model = Model(inputs=input, outputs=prediction)
    model.compile(loss='mean_squared_error', optimizer='adam', metrics=['accuracy'])

    # Display model summary
    model.summary()

    # TODO: Variable learning rate
    model.fit(train_dataset, validation_data=validation_dataset, epochs=epochs, callbacks=[tensorboard_callback])
    
    # Save the model
    model.save(model_file)

else:
    # Just load the model without training
    model = load_model(model_file)
    model.summary()

predictions = model.predict(validation_dataset)

print(predictions.shape)

for i in range(0, 10):
    print('\nPrediction ' + str(i) + ':')
    print(predictions[i])

'''
for i in range(0, 10):
    print('\nPrediction ' + str(i) + ':')
    print(predictions[i])
    print(train_dataset.y[i])
    print(np.linalg.norm(train_dataset.y[i]-predictions[i], np.inf))
    # Write predictions in files
    filepath = os.path.join(testset_folder, "{:d}.txt".format(i))
    with open(filepath, "w") as file:
        file.write("{:.2f}\n".format(predictions[i][0]))
        file.write("{:.2f}\n".format(predictions[i][1]))
        file.write("{:.2f}\n".format(predictions[i][2]))
        file.write("{:.2f}\n".format(predictions[i][3]))
        file.write("{:.2f}\n".format(predictions[i][4]))
        file.write("{:.2f}\n".format(predictions[i][5]))

# Display average infinity norm over the whole dataset
mean = 0.0
for i in range(0, len(predictions)):
    mean += np.linalg.norm(train_dataset.y[i]-predictions[i], np.inf)
mean /= len(predictions)
print('\n\nAverage infinity norm: ' + str(mean))
'''