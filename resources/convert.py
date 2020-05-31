import math
import numpy as np
from PIL import Image
from argparse import ArgumentParser
import matplotlib.pyplot as plt
import sys
import os.path
import re


def image_to_char_array(image):
    assert (len(image.shape) == 2)

    height, width = image.shape

    el = int(math.ceil(width * height / 8))
    ret = np.zeros(el, dtype=int)

    for y in range(0, height):
        c = y * width
        for x in range(0, width):
            r = int((x + c) / 8)
            if image[y, x] >= 128:
                ret[r] |= 0x80 >> ((c + x) % 8)
            else:
                ret[r] &= ~(0x80 >> ((c + x) % 8))
    return ret


def char_array_to_image(chars, height):
    width = int(len(chars) * 8 / height)
    height = int(height)

    img = np.zeros((height, width), dtype=int)

    for y in range(0, height):
        c = y * width
        for x in range(0, width):
            r = int((x + c) / 8)

            if int(chars[r]) & (0x80 >> ((c + x) % 8)) > 0:
                img[y, x] = 1
    return img


def char_array_to_png(chars, height, alpha=None):
    image = char_array_to_image(chars, height)
    result = np.dstack((image, image, image, np.ones(image.shape)))

    if alpha is not None:
        alpha_channel = char_array_to_image(alpha, height)
        result[:, :, 3] = alpha_channel

    return result.astype(np.uint8) * 255


def convert_image(path, show_output=False):
    image = np.array(Image.open(path))

    assert (image is not None)  # Able to load image?
    assert (image.dtype == np.uint8)  # Image format is correct? (otherwise logic further down might not be correct)

    height = image.shape[0]
    width = image.shape[1]

    alpha = None

    if len(image.shape) > 2:
        # only create alpha channel if any element is 0
        if image.shape[2] > 3 and np.any(image[:, :, 3] < 128):
            alpha = image_to_char_array(image[:, :, 3])

        image = image[:, :, 0]

    arr = image_to_char_array(image)

    nn = os.path.basename(path).split('.')[0].upper()
    print('const int ' + nn + '_INFO[] = {' + str(width) + ',' + str(height) + ',0,0};')
    print('const unsigned char ' + nn + '[] PROGMEM = {' + ''.join('0X{:02X},'.format(x) for x in arr) + '};')
    if alpha is not None:
        print(
            'const unsigned char ' + nn + '_ALPHA[] PROGMEM = {' + ''.join('0X{:02X},'.format(x) for x in alpha) + '};')

    if show_output:
        res = char_array_to_png(arr, height, alpha)
        fig, ax = plt.subplots(nrows=1, ncols=1)
        ax.set_facecolor('xkcd:pale green')
        ax.imshow(res)


def convert_font(path, letter_codes, show_output=False):
    font = np.array(Image.open(path))

    assert (font is not None)  # Able to load image?
    assert (font.dtype == np.uint8)  # Image format is correct? (otherwise logic further down might not be correct)
    assert (len(font.shape) == 3)  # Needs multiple channels
    assert (font.shape[2] == 4)  # Needs alpha channel

    height, width, _ = font.shape

    letters = []
    letter_chars = []

    free = np.all(font[:, :, 3] == 0, axis=0)

    current_letter_start = -1
    for x in range(0, width):
        if current_letter_start < 0 and not free[x]:
            current_letter_start = x
        elif current_letter_start >= 0 and free[x]:
            letters.append((current_letter_start, x))
            current_letter_start = -1

    assert (current_letter_start >= 0)
    assert len(letters) == len(letter_codes), str(len(letters)) + '<>' + str(len(letter_codes))

    width_condensed = 0
    for letter in letters:
        width_condensed = width_condensed + letter[1] - letter[0]

    font_condensed = np.zeros((height, width_condensed), np.uint8)

    letter_chars.extend([len(letters), width_condensed, height])

    x = 0
    for i in range(0, len(letters)):
        letter = letters[i]
        code = letter_codes[i]
        spacing = current_letter_start - letter[1]
        if i + 1 < len(letters):
            spacing = letters[i + 1][0] - letter[1]

        letter_chars.extend([code, x, letter[1] - letter[0], spacing])

        font_condensed[:, x:x + letter[1] - letter[0]] = font[:, letter[0]:letter[1], 0]
        x = x + letter[1] - letter[0]

    nn = os.path.basename(path).split('.')[0].upper()
    arr = image_to_char_array(font_condensed)

    print('// ' + nn + '_INFO: num of letters, width, height, (char code, x, width, spacing)')
    print('const int ' + nn + '_INFO[] PROGMEM = {' + ','.join([str(x) for x in letter_chars]) + '};')
    print('const unsigned char ' + nn + '[] PROGMEM = {' + ''.join('0X{:02X},'.format(x) for x in arr) + '};')

    if show_output:
        res = char_array_to_png(arr, height, None)
        fig = plt.figure(figsize=(16, 8))
        ax = fig.gca()
        ax.set_facecolor('xkcd:pale green')
        ax.imshow(res)


def print_characters(name, chars, show_output = False):
    letter_codes = [ord(x) for x in list(chars)]
    return convert_font(name, letter_codes, show_output)


class Console(object):
    def __init__(self):
        parser = ArgumentParser(description='Convert to byte arrays', usage='''python convert.py <command> [<args>]
commands:
    image <files>                   Convert all images and return in byte arrays
    font <file> "<letter string>"   Convert image and letters into font description
    preview <file>            Preview a byte array in file''')

        parser.add_argument('command', choices=['image', 'font', 'preview'])
        args = parser.parse_args(sys.argv[1:2])
        getattr(self, args.command)(sys.argv[2:])

    def image(self, argv):
        parser = ArgumentParser(description='Convert all images and return in byte arrays')
        parser.add_argument('--show', action='store_true')
        parser.add_argument('files', help='files', nargs='+')

        args = parser.parse_args(argv)

        files = []
        for path in args.files:
            path = os.path.abspath(path)
            if os.path.isdir(path):
                for p, d, f in os.walk(path):
                    # path, directories, files
                    files.extend([os.path.join(p, file) for file in f])
            else:
                files.append(path)

        for file in files:
            convert_image(file, args.show)
            print('')

    def font(self, argv):
        parser = ArgumentParser(description='Convert all images and return in byte arrays')
        parser.add_argument('file', type=str)
        parser.add_argument('--show', action='store_true')
        parser.add_argument('letters', type=str, nargs='+')

        args = parser.parse_args(argv)

        letters = ' '.join(args.letters)
        return convert_font(os.path.abspath(args.file), [ord(x) for x in list(letters)], args.show)

    def preview(self, argv):
        parser = ArgumentParser(description='Preview a byte array')
        parser.add_argument('-height', type=int)
        parser.add_argument('file', type=str)

        args = parser.parse_args(argv)
        
        if not os.path.isfile(os.path.abspath(args.file)):
            raise ValueError('File not found: ' + args.file)
        
        f = open(os.path.abspath(args.file), 'r')
        b = f.read()
        f.close()

        a1 = re.search('\{?((?:0X[0-9A-F]{2},?)+)\}?', b)
        a2 = re.search('((?:[0-9]{1,3}\s*)+)', b)
        
        if a1 is not None:
            res = char_array_to_png([int(x.lower(), 16) for x in a1.group(1).split(',') if len(x) > 0], args.height, None)
        elif a2 is not None:
            print(len(a2.group(1).split(' ')))
            res = char_array_to_png([x for x in a2.group(1).split(' ') if len(x) > 0], args.height, None)
        else:
            raise ValueError('Can\'t find {0X..,0X..,...} sequence in provided string ' + b)
        fig = plt.figure(figsize=(16, 8))
        ax = fig.gca()
        ax.set_facecolor('xkcd:pale green')
        ax.imshow(res)
        plt.show()


if __name__ == '__main__':
    Console()
