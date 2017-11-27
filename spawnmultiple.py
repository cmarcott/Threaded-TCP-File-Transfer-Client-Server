#!/usr/bin/python
import os
import sys
from subprocess import call

if len(sys.argv) < 2:
	print "\nPlease enter an argument\n"
	print "Eg. Correct Syntax for 3 clients connecting to localhost port 1234 would be:\n  './spawnmultiple.py 3 localhost:1234'"
	print "Please try Again\n"
	exit()

num_clients = sys.argv[1]


try:
    num_clients = int(num_clients)
except ValueError:
    print "\n**Please enter an integer representing the amount of clients as the first parameter\n"
    print "Eg. Correct Syntax for 3 clients connecting to localhost port 1234 would be:\n  './spawnmultiple.py 3 localhost:1234'"
    print "Please try Again\n"
    exit()


if len(sys.argv) > 2:
	execute_string = "./client " + sys.argv[2]
if len(sys.argv) > 3:
	execute_string += " " + sys.argv[3]
if len(sys.argv) > 4:
	execute_string += " " + sys.argv[4]
if len(sys.argv) > 5:
	execute_string += " " + sys.argv[5]
if len(sys.argv) > 6:
	execute_string += " " + sys.argv[6]
if len(sys.argv) > 7:
	execute_string += " " + sys.argv[7]

final_execute_string = execute_string
for i in range(1, num_clients):
	final_execute_string += " & "
	final_execute_string += execute_string

print final_execute_string
os.system(final_execute_string)