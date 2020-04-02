import numpy as np

from os.path import isfile, join

from Images import Images

class Dataset(object):
    """
    Manage the dataset:
        load, preprocess, split train and test set
    """

    def __init__(self, img_rows, img_cols, shuffle):
        # Target size of images
        self.target_size=(img_rows, img_cols)
        # If true, shuffle the dataset
        self.shuffle = shuffle
        # Training data set
        self.x = []
        self.y = np.empty((0))


    def load_folder(self, directory_path):
        # Load real images
        image_files = Images.list_images_directory(directory_path)
        images = Images.read_list(directory_path, image_files, self.target_size)
        # Preprocessing
        images = Images.preprocess(images)
        # Shuffle the dataset
        if self.shuffle:
            indices = np.arange(images.shape[0])
            np.random.shuffle(indices)
            image_files = image_files[indices]
            images = images[indices]

        self.x = images

        # Load text files
        txt_files = Images.list_texts_directory(directory_path)
        self.y = Images.read_list_txt(directory_path, txt_files)
        

    def load_image(self, image_path):
        # Read the image
        image = Images.read(image_path)
        # Preprocessing
        image = Images.preprocess(image)
        self.files = [image_path]
        self.x = image


    def print_information(self):
        # Print information on the dataset
        print('Dataset shape:', self.x.shape)
        print(self.x.shape[0], 'train samples')
        print(self.x.shape)
        print(self.y.shape)
        print('No test samples')
