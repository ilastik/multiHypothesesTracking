#!/usr/bin/python

from random import random
from h5py import *

numFeatures=20000

f = File("training_dataset.hdf", "w")

f.create_dataset("num_nodes", (1,), dtype='i', data=[10])

f.create_dataset("features_cover", (10, numFeatures), dtype='d', data=[
    [ [ random() for i in range(0,numFeatures) ] for j in range(0,10) ]
])

# node ids:
#             0
#           / | \
#          1  2  3
#         / \   / \
#        4   5 6   7
#        |   |
#        8   9
#
f.create_dataset("child_edges", (9,2), dtype='i', data=[
    [0,1],
    [0,2],
    [0,3],
    [1,4],
    [1,5],
    [3,6],
    [3,7],
    [4,8],
    [5,9]
])

# labels:
#             9
#           / | \
#          2  2  4
#         / \   / \
#        0   2 1   2
#        |   |
#        0   1
f.create_dataset("labels", (10,), dtype='i', data=[
    9, 2, 2, 4, 0, 2, 1, 2, 0, 1
])
