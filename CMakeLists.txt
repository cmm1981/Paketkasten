#
# Copyright (c) 2025 Conny Marco Menebröcker
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(application)

target_sources(app PRIVATE src/main.c src/motor.c src/inputs.c src/states.c)