# Empty stub for espressif/esp_rainmaker. The real component is pulled
# in transitively by espressif/arduino-esp32, but BTClock doesn't use
# any RainMaker feature (we have CONFIG_ARDUINO_SELECTIVE_RainMaker=n in
# sdkconfig.defaults). Without this override the Component Manager
# still downloads and builds it, embedding ~40 KiB of server certs
# (rmaker_mqtt_server.crt.S / rmaker_claim_service_server.crt.S /
# rmaker_ota_server.crt.S) into the firmware image.
#
# Wired through main/idf_component.yml via:
#   espressif/esp_rainmaker:
#     version: "*"
#     override_path: "./component_overrides/esp_rainmaker"

idf_component_register()
