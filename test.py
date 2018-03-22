import subprocess as sp
import sys
import time


process = sp.Popen(['./client', 'localhost'], stdin=sp.PIPE, stdout=None, stderr=sp.PIPE)
data = process.communicate(input='1')
data = process.communicate(input='1')

data = process.communicate(input='e')

print(data)
