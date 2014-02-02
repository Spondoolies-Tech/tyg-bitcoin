#!/bin/sh

# Rootfs customization.

# This is provided by Buildroot
IMAGE_DIR=$1

lzma -9f ${IMAGE_DIR}/rootfs.cpio
