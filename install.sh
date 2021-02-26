#!/bin/bash

INSTALL_DIR=/usr/local
LIB_DIR=lib
H_DIR=include
PKG_DIR=/usr/lib/pkgconfig

if [ "$EUID" -ne 0 ]
then
  echo "Please run as root."
  exit 1
fi

echo "Building and installing unyte shared lib in: $INSTALL_DIR/$LIB_DIR"

make build

echo "Moving build file to $INSTALL_DIR/$LIB_DIR"
cp libunyte.so $INSTALL_DIR/$LIB_DIR

if [ $? -ne 0 ]
then
  echo "Could not move shared lib to $INSTALL_DIR/$LIB_DIR" >&2
  echo "Try sudo"
  exit 1
fi

if [ ! -d "$INSTALL_DIR/$H_DIR/unyte" ]
then
  echo "Creating $INSTALL_DIR/$H_DIR/unyte directory"
  mkdir -p $INSTALL_DIR/$H_DIR/unyte
fi

echo "Copying headers to $INSTALL_DIR/$H_DIR/unyte"
cp src/*.h $INSTALL_DIR/$H_DIR/unyte

if [ $? -ne 0 ]
then
  echo "Error copying headers" >&2
  echo "Try sudo"
  exit 1
fi

echo "Copying pkg-config file to $PKG_DIR"
sed -e "s/<<install>>/${INSTALL_DIR//\//\\/}/g" -e "s/<<include>>/$H_DIR/g" -e "s/<<lib>>/$LIB_DIR/g" unyte-pkg.pc > unyte.pc
cp unyte.pc $PKG_DIR

if [ $? -ne 0 ]
then
  echo "Error copying pkg-config file to $PKG_DIR" >&2
  echo "Try sudo"
  exit 1
fi