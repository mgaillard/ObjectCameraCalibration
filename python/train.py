import os 
import datetime
from time import time

import tensorflow as tf
from tensorflow import keras

from tensorflow.keras.datasets import mnist
from tensorflow.keras.models import Model, Sequential 
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
input_shape = (img_rows, img_cols, 3)
batch_size = 256
epochs = 300
dataset_folder = "C:\\Users\\gaill\\Desktop\\dataset"
# dataset_folder = "/mnt/c/Users/gaill/Desktop/dataset"

# Load dataset
train_dataset = Dataset(img_rows, img_cols, False)
train_dataset.load_folder(dataset_folder)
train_dataset.print_information()

# Define the tensors for the two input images
input = Input(shape=input_shape)
x = Conv2D(4, (3, 3), activation='relu', padding='same')(input)
x = Conv2D(4, (3, 3), activation='relu', padding='same')(x)
x = MaxPooling2D((2, 2), strides=(2, 2))(x)
x = Conv2D(4, (3, 3), activation='relu', padding='same')(x)
x = Conv2D(4, (3, 3), activation='relu', padding='same')(x)
x = MaxPooling2D((2, 2), strides=(2, 2))(x)
x = Conv2D(4, (3, 3), activation='relu', padding='same')(x)
x = Conv2D(4, (3, 3), activation='relu', padding='same')(x)
x = Flatten()(x)
x = Dense(16, activation='relu')(x)
x = Dense(16, activation='relu')(x)
prediction = Dense(3, activation='linear')(x)

model = Model(inputs=input, outputs=prediction)

model.compile(loss='mean_squared_error', optimizer='adam', metrics=['accuracy'])
model.summary()

tensorboard = TensorBoard(log_dir="logs/{}".format(time()))

model.fit(x=train_dataset.x,
          y=train_dataset.y,
          batch_size=batch_size,
          epochs=epochs,
          validation_split=0.2,
          shuffle=True,
          callbacks=[tensorboard_callback])

          
predictions = model.predict(train_dataset.x)

for i in range(0, 3):
    print('\nPrediction ' + str(i) + ':')
    print(predictions[i])
    print(train_dataset.y[i])
    print(np.linalg.norm(train_dataset.y[i]-predictions[i], np.inf))

# Display average infinity norm over the whole dataset
mean = 0.0
for i in range(0, len(predictions)):
    mean += np.linalg.norm(train_dataset.y[i]-predictions[i], np.inf)
mean /= len(predictions)
print('\n\nAverage infinity norm: ' + str(mean))