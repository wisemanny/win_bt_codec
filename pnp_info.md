# Use of PNP Device

Looking at the disassembly of bth2adp.sys I can see that at least on Windows 11, they store the current set of codecs in the PNP *device* interface. The codecs and other data is stored under the GUID - 29ce83d4-7a82-4744-bd1d-abec85321dd6

Here is an example of my output:

```
pnputil /enum-devices /connected /interfaces /properties /class MEDIA


Interface Path:         \\?\BTHENUM#{0000110b-0000-1000-8000-00805f9b34fb}_VID&00010ad2_PID&0038#5&1cd044e&0&7C96D2F479F4_C00000000#{6994ad04-93ef-11d0-a3cc-00a0c9223196}\SRC
    Interface Description:  Unknown
    Interface Class GUID:   {6994ad04-93ef-11d0-a3cc-00a0c9223196}
    Reference String:       SRC
    Interface Status:       Enabled
    Interface Properties:
        DEVPKEY_DeviceInterface_Enabled [Boolean]:
            TRUE
        DEVPKEY_Device_InstanceId [String]:
            BTHENUM\{0000110b-0000-1000-8000-00805f9b34fb}_VID&00010ad2_PID&0038\5&1cd044e&0&7C96D2F479F4_C00000000
        DEVPKEY_DeviceInterface_ClassGuid [GUID]:
            {6994ad04-93ef-11d0-a3cc-00a0c9223196}
        DEVPKEY_DeviceInterface_ReferenceString [String]:
            SRC
        DEVPKEY_Device_ContainerId [GUID]:
            {e77ebc75-8ba2-5230-8c37-80a86d214500}
        DEVPKEY_KsAudio_Controller_DeviceInterface_Path [String]:
            \??\BTHENUM#{0000110b-0000-1000-8000-00805f9b34fb}_VID&00010ad2_PID&0038#5&1cd044e&0&7C96D2F479F4_C00000000#{e630310c-1733-42bb-9847-7e45d1a27490}\CONTROL
        {194ef948-7cdb-403e-9f47-19418f7b24fd}[1] [UINT64]:
            0x000000000A1E3326 (169751334)
        {29ce83d4-7a82-4744-bd1d-abec85321dd6}[2] [Boolean]:
            TRUE
        {29ce83d4-7a82-4744-bd1d-abec85321dd6}[3] [Binary]:
            02
        {29ce83d4-7a82-4744-bd1d-abec85321dd6}[4] [Binary]:
            FF D7 00 00 00 24 00 FF 4F 00 00 00 01 00 02 FF
            0A 00 00 00 06 01 FF 0A 00 00 00 04 01 FF 0A 00
            00 00 03 01 00
        DEVPKEY_Bluetooth_DeviceAddress [String]:
            7C96D2F479F4
        DEVPKEY_Bluetooth_ServiceGUID [GUID]:
            {0000110b-0000-1000-8000-00805f9b34fb}
        {7fb7b48f-531d-44a2-bcb3-5ad5a134b3dc}[131073] [Binary]:
            B8 9C 9E C1 B8 9C 9E C1
```

In the code, I see that Windows uses this GUID for such values (I also added how they store it, to see the size):

DEVPKEY_Bluetooth_ConfiguredCodec - saved as vector with variable length
DEVPKEY_Bluetooth_LocalControllerSupportedCodecs  - saved as vector with variable length
DEVPKEY_Bluetooth_RemoteSupportedCodecs - saved as vector with variable length
DEVPKEY_Bluetooth_IsAvdtpConnected - size 1
DEVPKEY_Bluetooth_StreamDisposition  - size 4

And indeed, if I take one of the values, I can decode it into the codecs supported by the speaker:

```
        {29ce83d4-7a82-4744-bd1d-abec85321dd6}[4] [Binary]:
            FF D7 00 00 00 24 00 FF 4F 00 00 00 01 00 02 FF
            0A 00 00 00 06 01 FF 0A 00 00 00 04 01 FF 0A 00
            00 00 03 01 00
```

Here is how it can be split into the list of codecs:

FF  0xD7 0024  aptX HD
FF  0x4F 0001  aptX
02             MPEG-2,4 (aka AAC)  <--- Currently used, seems like other value of 2 is index
FF 0xA 0x0106  CSR aptX
FF 0xA 0x0104  CSR AAC
FF 0xA 0x0103  Qualcomm
00
