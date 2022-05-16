# Automatic number plate recognition

## Requirements
- OpenCV
- Tesseract
- Leptunica

## Installation for arch

### Clone this repository:
`
git clone git@github.com:Snakehater/ANPR.git
`

### Install requirements:
`
sudo pacman -S opencv tesseract leptonica
`

## Setup for linux

### Inside the repository root:
`
mkdir build;
cd build;
cmake ..;
`

## Build

`
cd build;
make;
`

## Run
`
cd build;
./main;
`
