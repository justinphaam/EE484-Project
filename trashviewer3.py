import serial
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.cm as cm
from matplotlib.animation import FuncAnimation
from scipy.ndimage import zoom, gaussian_filter

# Set the serial port for communication with ESP32
ser = serial.Serial('COM4', 115200)  # Adjust to your serial port
ser.flushInput()  # Clear the serial input buffer

# Initialize a global variable to hold temperature data
temperature_data = np.zeros((12, 16))

# Function to read and parse temperature data
def get_temperature_data():
    line = ser.readline().decode('utf-8').strip()  # Read one line from serial and strip whitespace
    print(f"Received line: {line}")  # Debugging: print received line
    if not line:
        print("Empty line received.")  # Debugging: check for empty lines
        return None

    try:
        # Convert CSV values to a list of floats, while skipping empty values
        temp_data = [float(x) for x in line.split(',') if x.strip()]  # Filter out empty values
        if len(temp_data) != 192:
            print(f"Invalid data length, expected 192 values, got {len(temp_data)}.")  # Debugging: invalid length
            return None
        return np.array(temp_data).reshape((12, 16)).T  # Reshape and transpose to correct orientation
    except ValueError as e:
        print(f"Failed to parse data: {e}")  # Debugging: more detailed error message
        return None

# Function to update the image for the animation
def update(frame):
    global temperature_data
    # Get the latest temperature data from the ESP32
    new_data = get_temperature_data()
    if new_data is not None:
        temperature_data = new_data.T  # Transpose the data for correct orientation
        print("Data updated.")  # Debugging: confirm data update

    # Apply bicubic interpolation to upscale resolution and Gaussian filter to smooth the image
    interpolated_data = zoom(temperature_data, (4, 4), order=3)  # 4x zoom with bicubic interpolation
    smoothed_data = gaussian_filter(interpolated_data, sigma=1.0)  # Apply Gaussian smoothing

    # Clip values to ensure that they don't exceed the vmax limit
    smoothed_data = np.clip(smoothed_data, None, 100)  # Limit to vmax (100°C)

    # Update the image
    im.set_array(smoothed_data)
    return [im]

# Set up the plot
fig, ax = plt.subplots()
im = ax.imshow(temperature_data, cmap=cm.jet, vmin=10, vmax=100)  # Set vmin and vmax to the expected temperature range
ax.set_title('Thermal Image')
cbar = fig.colorbar(im, ax=ax)
cbar.set_label('Temperature (°C)')

# Set up the animation
ani = FuncAnimation(fig, update, blit=False, interval=125, frames=None, cache_frame_data=False)  # Update every 125 ms

# Display the plot
plt.show()
