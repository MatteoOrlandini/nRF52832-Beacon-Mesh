# Beacon Mesh Application

This example demonstrates how [nRF5 SDK for Mesh v4.0.0](https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.meshsdk.v4.0.0/index.html?cp=7_6_0) and [nRF5 SDK examples v16.0.0](https://infocenter.nordicsemi.com/topic/sdk_nrf5_v16.0.0/index.html?cp=7_5_0) can be used together
at the same time on a nRF52832 board. It is built around two examples and has three parts:
* The mesh part of this example implements
    * the [light switch client example](https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.meshsdk.v4.0.0/md_examples_light_switch_README.html?cp=7_6_0_3_0) 
    * and the [beaconing example](https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.meshsdk.v4.0.0/md_examples_beaconing_README.html?cp=7_6_0_3_2) from the nRF5 SDK for Mesh.
* The BLE part of the example implements the [Eddystone Beacon Application](https://infocenter.nordicsemi.com/topic/sdk_nrf5_v16.0.0/ble_sdk_app_es.html?cp=7_5_0_4_2_2_9) example from the nRF5 SDK.

As the result of running this example, you'll be able to use a mesh network in which this example can replace the light switch client example. 


---

# Prerequisites

* Install [IDE Segger Embedded Studio](https://www.segger.com/downloads/embedded-studio/)
* Download [nRF5 SDK examples v16.0.0](https://infocenter.nordicsemi.com/topic/sdk_nrf5_v16.0.0/index.html?cp=7_5_0) and extract the zip file into the directory you want to use to work with the SDK.
* Download [nRF5 SDK for Mesh v4.0.0](https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.meshsdk.v4.0.0/index.html?cp=7_6_0) and extract the zip file in the directory you want to use to work with the SDK. It's recommended to extract the content to the same folder as the SDK downloaded to the previous step.

You can copy the content of this repository to `<InstallFolder>/examples/ble_peripheral`

---

## Testing the firmware

1. Copy this repository folder into the `examples/ble_peripheral` folder
at your nRF5 SDK installation path.
2. Open the Segger Embedded Studio project at `/pca10040/s132/ses/ble_app_proximity_pca10040_s132.emProject`.
3. Add MESH_ROOT to your Segger Embedded Studio global macro list:
    1. From the SES menu bar, click **Tools > Options...**.
    2. In the left column, click **Building**.
    3. In right column, double click **Global macros**.
    4. Add your mesh root directory in a new line: `MESH_ROOT=<path to your mesh installation>`.
4. Ensure that the flash is completely erased before programming this example.
Otherwise Flash Data Storage module will not work correctly.
5. Program the light switch example devices with the following exception:
    * Replace the light switch client with `/pca10040/s132/ses/Output/Release/Exe/ble_app_proximity_pca10040_s132.hex`.
    
---

# Results

With this application the board broadcasts three kinds of beacon (nRF Beacon, iBeacon and Eddystone URL Beacon) while partecipating in a mesh network. The user can also connect to the board and change the Eddystone Beacon via the Eddystone Configuration Service.

You can test Bluetooth Mesh functionalities with two other boards that implement the Provisioner and Server role.

You can test beacons and BLE GATT with [nRF Connect](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp&hl=it) and then you can see the three kinds of beacon transmitted as shown in the images below.

![](https://github.com/MatteoOrlandini/nRF52832-Beacon-Mesh/blob/master/images/Eddystone.PNG)

![](https://github.com/MatteoOrlandini/nRF52832-Beacon-Mesh/blob/master/images/iBeacon.PNG)

![](https://github.com/MatteoOrlandini/nRF52832-Beacon-Mesh/blob/master/images/nRF_Beacon.PNG)

If you click 'Connect' you can see the Eddystone Configuration Service.

![](https://github.com/MatteoOrlandini/nRF52832-Beacon-Mesh/blob/master/images/ESCS.jpg)

---

# Extra

For more information please see [Presentazione.pdf](https://github.com/MatteoOrlandini/nRF52832-Beacon-Mesh/blob/master/Presentazione.pdf) and [Relazione.pdf](https://github.com/MatteoOrlandini/nRF52832-Beacon-Mesh/blob/master/Relazione.pdf) in Italian. The English [Report.pdf](https://github.com/MatteoOrlandini/nRF52832-Beacon-Mesh/blob/master/Report.pdf) is under translation.
