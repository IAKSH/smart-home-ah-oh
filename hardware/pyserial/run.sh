#!/bin/bash

if [ ! -d "venv" ]; then
    echo "Virtual environment not found, creating it..."
    python3 -m venv venv
fi

source venv/bin/activate

echo "Upgrading pip and installing required libraries..."
pip install --upgrade pip
pip install pyserial

echo "Starting the Python script..."
python serial_tool.py

deactivate
