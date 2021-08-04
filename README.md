# FOTA 
Formware over the air transmission using HTTP Multipart from. 


## Build


### Prerequisites

```bash
sudo apt install sassc cleancss
```

### Environment

Follow [this](https://github.com/pylover/esp8266-env) instruction 
to setup your environment.

```bash
cd esp-env
source activate.sh
cd ..

git clone --recursive https://github.com/pylover/esp8266-fota.git
cd esp8266-fota 
make clean
make flash_map6webui
make flash_map6user1 
```
