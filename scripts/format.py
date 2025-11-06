#!/usr/bin/env python3
import os
import subprocess

extensions = ('.c', '.cpp', '.h', '.hpp')
directories = ['./src', './include', './processors', './tests']

for directory in directories:
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(extensions):
                filepath = os.path.join(root, file)
                subprocess.run(['clang-format', '-i', filepath])
