# Midterm Project - Implement ls(1)

## Student Information

- Name: Huynh Diep
- Student ID: 24IT039
- Course: Advanced Programming in UNIX Environment
- Project: Simplified implementation of `ls(1)`

## Introduction

This project implements a simplified version of the UNIX `ls(1)` command in C.

The implementation is based on the provided NetBSD `ls(1)` manual page and uses UNIX/POSIX filesystem functions such as:

- `opendir()`
- `readdir()`
- `closedir()`
- `stat()`
- `lstat()`
- `readlink()`
- `getpwuid()`
- `getgrgid()`
- `localtime_r()`

The program supports file and directory operands, sorting, recursive directory traversal, long-format output, inode information, block information, symbolic links, and several other options.

## Project Structure

```text
HuynhDiep_24IT039_midterm/
├── include/
│   ├── display.h
│   ├── listing.h
│   ├── options.h
│   └── sort.h
│
├── src/
│   ├── display.c
│   ├── listing.c
│   ├── main.c
│   ├── options.c
│   └── sort.c
│
├── Makefile
├── README.md
└── .gitignore
## GitHub Repository

https://github.com/dh006/HuynhDiep_24IT039_midterm
