#!/bin/bash
docker run -it \
    -v ./app:/home/hispark/hi3861_hdu_iot_application/src/applications/sample/wifi-iot/app \
    -v ./out:/home/hispark/hi3861_hdu_iot_application/src/out \
    hi3861-dev