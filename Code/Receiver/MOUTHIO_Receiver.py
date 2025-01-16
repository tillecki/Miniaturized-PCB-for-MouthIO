import sys
import asyncio
import logging

from bleak import BleakClient, BleakScanner
from bleak.backends.characteristic import BleakGATTCharacteristic

from PyQt5.QtCore import QThread, pyqtSignal
from PyQt5.QtWidgets import QApplication, QWidget, QLabel, QVBoxLayout
from pyqtgraph.Qt import QtWidgets, QtCore
import pyqtgraph as pg

from collections import deque

import struct


# Define UUIDs for characteristics
BATVOL_UUID     = "b66780c6-4a43-4ed6-85b2-535e244e1375"
CAPSENSE_UUID  = "fcc60129-6a4c-4600-9588-2f1d35b925d7"
ACC_UUID        = "269bdc96-e17a-4d14-805f-d5bfeb6504fb"

# Map UUIDs to human-readable names
uuid_to_name = {
    BATVOL_UUID: "BattVol",
    CAPSENSE_UUID: "CapSense",
    ACC_UUID: "ACC",
}

# Define device name
device_name = "MOUTHIO"

# Configure logging
logger = logging.getLogger(__name__)
log_level = logging.INFO  # Adjust log level (INFO or DEBUG)
logging.basicConfig(
    level=log_level,
    format="%(asctime)-15s %(name)-8s %(levelname)s: %(message)s",
)

# Set maximum number of data points to display (only show the latest N points)
max_points = 100

# Initialize data buffers for each characteristic
data_buffers = {
    "BattVol": deque(maxlen=max_points),
    "CapSense_1": deque(maxlen=max_points),
    "CapSense_2": deque(maxlen=max_points),
    "ACC_X": deque(maxlen=max_points),
    "ACC_Y": deque(maxlen=max_points),
    "ACC_Z": deque(maxlen=max_points),
}

def notification_handler(characteristic: BleakGATTCharacteristic, data: bytearray):
    logger.info("%s: %r", characteristic.uuid, data)
    uuid = characteristic.uuid
    if uuid in uuid_to_name:
        name = uuid_to_name[uuid]
        
        if name == "BattVol":
            value = int.from_bytes(data, "little")
            data_buffers[name].append(value)
        
        elif name == "ACC":
            x, y, z = struct.unpack('<fff', data[:12])
            data_buffers["ACC_X"].append(x)
            data_buffers["ACC_Y"].append(y)
            data_buffers["ACC_Z"].append(z)
        
        elif name == "CapSense":
            cap1, cap2 = struct.unpack('<II', data[:8])
            data_buffers["CapSense_1"].append(cap1)
            data_buffers["CapSense_2"].append(cap2)
    
    else:
        logger.warning("UUID not recognized, performing default action")

async def connect_and_subscribe(device):
    async with BleakClient(device) as client:
        logger.info("Connected")

        await client.start_notify(CAPSENSE_UUID, notification_handler)
        await client.start_notify(ACC_UUID, notification_handler)
        while(True):
            async def read_characteristic(client):
                while True:
                    try:
                        data = await client.read_gatt_char(BATVOL_UUID)
                        # Process the data here
                        value = struct.unpack('<f', data[:4])[0]
                        logger.info("READ: %b %f", data, value)

                        data_buffers["BattVol"].append(value)
                        #print(data)  # Example: print the raw data
                    except Exception as e:
                        logger.info("Error reading characteristic: {e}")
                    await asyncio.sleep(5)  # Wait for 5 seconds
            await read_characteristic(client)

async def find_and_connect():
    device = await BleakScanner.find_device_by_name(device_name)
    if device is None:
        logger.error("could not find device with name '%s'", device_name)
        return

    await connect_and_subscribe(device)

class BLEThread(QThread):
    data_received = pyqtSignal(dict)  # Signal to emit BLE dataç

    def run(self):
        asyncio.run(find_and_connect())

class MyWidget(QWidget):
    def __init__(self):
        super().__init__()

        self.label = QLabel()

        # Create BLE thread
        self.ble_thread = BLEThread()
        #self.ble_thread.data_received.connect(self.update_ui)       
        self.ble_thread.start()

        # PyQtGraph setup
        # Create a window with multiple plots
        win = pg.GraphicsLayoutWidget()  # Remove show=True to embed in QWidget

        # Create plot objects for each characteristic
        plots = {
            "BattVol": win.addPlot(title="BattVol"),
            "CapSense_1": win.addPlot(title="CapSense 1"),
            "CapSense_2": win.addPlot(title="CapSense 2"),
            "ACC_X": win.addPlot(title="ACC X"),
            "ACC_Y": win.addPlot(title="ACC Y"),
            "ACC_Z": win.addPlot(title="ACC Z"),
        }

        # Create a layout for the widget
        layout = QVBoxLayout()
        layout.addWidget(win)
        self.setLayout(layout)

        # Set window properties
        self.setWindowTitle("Real-Time Data from {device_name}")
        self.resize(1500, 600)

        # Initialize curve objects for each plot
        self.curves = {name: plot.plot() for name, plot in plots.items()}
        
      

    def update_plot(self):
        for name, buffer in data_buffers.items():
            self.curves[name].setData(list(range(len(buffer))), list(buffer))

    
        

if __name__ == '__main__':
    app = QApplication(sys.argv)
    widget = MyWidget()
    widget.show()
    # PyQtGraph timer for real-time updates
    timer = QtCore.QTimer()
    timer.timeout.connect(widget.update_plot)
    timer.start(50)  # Update every 50ms
    sys.exit(app.exec_())











# # Function to update the plots with new data
# def update_plot():
#     for name, buffer in data_buffers.items():
#         curves[name].setData(list(range(len(buffer))), list(buffer))

# # PyQtGraph timer for real-time updates
# timer = QtCore.QTimer()
# timer.timeout.connect(update_plot)
# timer.start(50)  # Update every 50ms
