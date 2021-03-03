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

echo "Removing $INSTALL_DIR/$LIB_DIR/libunyte-udp-notif.so"
rm $INSTALL_DIR/$LIB_DIR/libunyte-udp-notif.so

if [ $? -ne 0 ]
then
  echo "Could not remove shared lib to $INSTALL_DIR/$LIB_DIR" >&2
  echo "Try sudo"
  exit 1
fi

if [ -d "$INSTALL_DIR/$H_DIR/unyte-udp-notif" ]
then
  echo "Removing $INSTALL_DIR/$H_DIR/unyte-udp-notif directory"
  rm -r $INSTALL_DIR/$H_DIR/unyte-udp-notif
fi

if [ $? -ne 0 ]
then
  echo "Error Removing headers" >&2
  echo "Try sudo"
  exit 1
fi

echo "Removing pkg-config file $PKG_DIR/unyte-udp-notif.pc"
rm $PKG_DIR/unyte-udp-notif.pc

if [ $? -ne 0 ]
then
  echo "Error removing pkg-config file to $PKG_DIR" >&2
  echo "Try sudo"
  exit 1
fi

echo "/!\ You should remove the LD_LIBRARY_PATH manually from your .bashrc"
