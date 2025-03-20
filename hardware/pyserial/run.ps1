#!/usr/bin/env pwsh

Set-Location -Path $PSScriptRoot

$venvDir = "venv"
$activateScript = Join-Path -Path $venvDir -ChildPath "Scripts\Activate.ps1"

if (-Not (Test-Path $activateScript)) {
    Write-Output "Virtual environment not found, creating it..."
    python -m venv $venvDir
}

if (Test-Path $activateScript) {
    Write-Output "Activating the virtual environment..."
    . $activateScript
} else {
    Write-Error "Activation script not found. Virtual environment creation may have failed."
    exit 1
}

Write-Output "Upgrading pip and installing required libraries..."
pip install --upgrade pip
pip install pyserial

Write-Output "Running the Python script..."
python serial_tool.py
