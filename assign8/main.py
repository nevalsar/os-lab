#!/usr/bin/env python

import sys
from collections import deque
import matplotlib.pyplot as plt

def print_mat(mat, dimension, name):
    print "\nMatrix " + name + "\n"
    for x in xrange(dimension):
        for y in xrange(dimension):
            print(str(mat[x * dimension + y]) + " "),
        print
    print

def gen_reference_string(reference_string, dimension, page_size):
    A = []
    B = []
    C = []

    a_start = 101
    b_start = 501
    c_start = 1001
    reset = page_size

    for i in range(dimension*dimension):
        A.append(a_start)
        B.append(b_start)
        C.append(c_start)
        reset -= 1
        if reset == 0:
            a_start += 1
            b_start += 1
            c_start += 1
            reset = page_size

    reference_string = []
    for i in xrange(dimension):
        for j in xrange(dimension):
            for k in xrange(dimension):
                reference_string.append(A[i*dimension + k])
                reference_string.append(B[k*dimension + j])
            reference_string.append(C[i*dimension + j])

    print_mat(A, dimension, "A")
    print_mat(B, dimension, "B")
    print_mat(C, dimension, "C")
    return reference_string

def algorithm_fifo(reference_string, frame_size):
    page_fault = 0
    page_replacement = 0
    queue = deque([])
    for address in reference_string:
        if address not in queue:
            page_fault += 1
            if len(queue) >= frame_size:
                page_replacement += 1
                queue.popleft()
            queue.append(address)
    return page_fault

def algorithm_LRU(reference_string, frame_size):
    page_fault = 0
    page_replacement = 0
    stack = []
    for address in reference_string:
        if address in stack:
            stack.pop(stack.index(address))
            stack.append(address)
        else:
            page_fault += 1
            if len(stack) >= frame_size:
                page_replacement += 1
                stack.pop(0)
            stack.append(address)
    return page_fault

def algorithm_LFU(reference_string, frame_size):
    page_fault = 0
    page_replacement = 0
    stack = []
    for address in reference_string:
        address_index_list = [index for index, item in enumerate(stack) if item[0] == address]
        if len(address_index_list) > 1:
            print "LFU : Multiple instances of same address found in stack"
            exit(1)
        elif len(address_index_list) == 1:
            stack[address_index_list[0]][1] += 1
        else:
            page_fault += 1
            if len(stack) >= frame_size:
                page_replacement += 1
                min_reference = min(zip(*stack)[1])
                min_index = [index for index, item in enumerate(stack) if item[1] == min_reference]
                stack.pop(min_index[0])
            stack.append([address, 0])
    return page_fault

def algorithm_second_chance(reference_string, frame_size):
    page_fault = 0
    page_replacement = 0
    queue = []
    for address in reference_string:
        address_index_list = [index for index, item in enumerate(queue) if item[0] == address]
        if (len(address_index_list) > 1):
            print "Second Chance : Multiple instances of same address found in stack"
            exit(1)
        elif len(address_index_list) == 1:
            queue[address_index_list[0]][1] = 1
        else:
            page_fault += 1
            if len(queue) >= frame_size:
                page_replacement += 1
                suitable = -1;
                for i in range(len(queue)):
                    if queue[i][1] == 0:
                        suitable = i
                        break
                    else:
                        queue[i][1] = 0
                if suitable == -1:
                    suitable = 0
                queue.pop(suitable)
            queue.append([address, 0])
    return page_fault

def main():
    while True:
        choice = int(raw_input(
            """
-------------------------------
            MENU
-------------------------------
1. FIFO
2. LRU
3. LFU
4. Second Chance
5. All

0. Exit
-------------------------------
Enter choice : """
            ))

        if choice == 0:
            print "Bye"
            exit(0)

        dimension = int(raw_input("Enter dimension : "))
        page_size = int(raw_input("Enter page_size : "))

        # generate reference string
        reference_string = []
        reference_string = gen_reference_string(reference_string, dimension, page_size)

        # generate some frame sizes to plot graph for
        # no of individual values of page address = (dimension*dimension*3)/page_size
        frame_sizes = range(1, 2 * ((3 * dimension * dimension) / page_size))

        # FIFO
        if choice == 1 or choice == 5:
            faults_fifo = []
            for size in frame_sizes:
                faults_fifo.append(algorithm_fifo(reference_string, size));
            line, = plt.plot(frame_sizes, faults_fifo, 'r')
            line.set_label('FIFO')
            plt.legend()

        # LRU
        if choice == 2 or choice == 5:
            faults_LRU = []
            for size in frame_sizes:
                faults_LRU.append(algorithm_LRU(reference_string, size));
            line, = plt.plot(frame_sizes, faults_LRU, 'g')
            line.set_label('LRU')
            plt.legend()

        # LFU
        if choice == 3 or choice == 5:
            faults_LFU = []
            for size in frame_sizes:
                faults_LFU.append(algorithm_LFU(reference_string, size));
            line, = plt.plot(frame_sizes, faults_LFU, 'b')
            line.set_label('LFU')
            plt.legend()

        # Second chance
        if choice == 4 or choice == 5:
            faults_second_chance = []
            for size in frame_sizes:
                faults_second_chance.append(algorithm_second_chance(reference_string, size));
            line, = plt.plot(frame_sizes, faults_second_chance, 'y')
            line.set_label('Second chance')
            plt.legend()

        # Invalid choice
        if choice > 5 or choice < 0:
            print "Invalid choice. Try again."

        # display graph with axes annotations
        plt.ylabel('Page faults')
        plt.xlabel('Frame size')
        plt.show()



if __name__ == '__main__':
    main()
