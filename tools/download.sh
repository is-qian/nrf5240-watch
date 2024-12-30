#!/bin/bash

JLinkExe -device nRF5340_xxAA_APP -if SWD -speed 4000 -CommanderScript ./tools/flash.jlink
