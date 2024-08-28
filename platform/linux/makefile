INCLUDE_PATH := ./include
INCLUDE_PATH += ./shader/include
INCLUDE_PATH += ./platform/Vkinclude

LIBRARIES := X11
LIBRARIES += GL
LIBRARIES += pthread
LIBRARIES += asound

APP_DEPENDS := CrystalCore
APP_DEPENDS += CrystalAlgorithms
APP_DEPENDS += CrystalThread
APP_DEPENDS += CrystalAudio
APP_DEPENDS += CrystalGraphic

INCLUDE_PATH := $(foreach item,$(INCLUDE_PATH),-I$(item))

LIBRARIES := $(foreach item,$(LIBRARIES),-l$(item))
LIBRARIES += ./platform/linux/libvulkan.so

APP_DEPENDS := $(foreach item,$(APP_DEPENDS),-l$(item))

C_FILES := $(shell find . -name "*.c")
APP_FILES := $(filter ./application/%.c,$(C_FILES))
C_FILES_MOD := $(filter ./src/%.c,$(C_FILES))
C_FILES_CORE := $(filter ./core/%.c,$(C_FILES))
OBJ_FILES_MOD := $(patsubst ./src/%.c,./out/objs/src/%.o,$(C_FILES_MOD))
OBJ_FILES_CORE := $(patsubst ./core/%.c,./out/objs/core/%.o,$(C_FILES_CORE))

SHADER_FILES := $(shell find . -name "*.glsl")
OBJ_SHADER_FILES := $(patsubst ./%.glsl,./out/objs/%.o,$(SHADER_FILES))


C_MOD_FILES_CORE := $(filter ./src/CrystalCore/%.c,$(C_FILES_MOD))
C_MOD_FILES_ALG := $(filter ./src/CrystalAlgorithms/%.c,$(C_FILES_MOD))
C_MOD_FILES_THR := $(filter ./src/CrystalThread/%.c,$(C_FILES_MOD))
C_MOD_FILES_AUDIO := $(filter ./src/CrystalAudio/%.c,$(C_FILES_MOD))
C_MOD_FILES_GRC := $(filter ./src/CrystalGraphic/%.c,$(C_FILES_MOD))
OBJ_MOD_FILES_CORE := $(filter ./out/objs/src/CrystalCore/%.o,$(OBJ_FILES_MOD))
OBJ_MOD_FILES_ALG := $(filter ./out/objs/src/CrystalAlgorithms/%.o,$(OBJ_FILES_MOD))
OBJ_MOD_FILES_THR := $(filter ./out/objs/src/CrystalThread/%.o,$(OBJ_FILES_MOD))
OBJ_MOD_FILES_AUDIO := $(filter ./out/objs/src/CrystalAudio/%.o,$(OBJ_FILES_MOD))
OBJ_MOD_FILES_GRC := $(filter ./out/objs/src/CrystalGraphic/%.o,$(OBJ_FILES_MOD))

C_CORE_FILES_CORE := $(filter ./core/CrystalCore/%.c,$(C_FILES_CORE))
C_CORE_FILES_ALG := $(filter ./core/CrystalAlgorithms/%.c,$(C_FILES_CORE))
C_CORE_FILES_THR := $(filter ./core/CrystalThread/%.c,$(C_FILES_CORE))
C_CORE_FILES_AUDIO := $(filter ./coreCrystalAudio/%.c,$(C_FILES_CORE))
C_CORE_FILES_GRC := $(filter ./core/CrystalGraphic/%.c,$(C_FILES_CORE))
OBJ_CORE_FILES_CORE := $(filter ./out/objs/core/CrystalCore/%.o,$(OBJ_FILES_CORE))
OBJ_CORE_FILES_ALG := $(filter ./out/objs/core/CrystalAlgorithms/%.o,$(OBJ_FILES_CORE))
OBJ_CORE_FILES_THR := $(filter ./out/objs/core/CrystalThread/%.o,$(OBJ_FILES_CORE))
OBJ_CORE_FILES_AUDIO := $(filter ./out/objs/core/CrystalAudio/%.o,$(OBJ_FILES_CORE))
OBJ_CORE_FILES_GRC := $(filter ./out/objs/core/CrystalGraphic/%.o,$(OBJ_FILES_CORE))

clean:
	@if [ ! -d "./out/" ];then mkdir ./out;fi
	@if [ -d "./out/Linux/" ];then rm -r ./out/Linux/;fi
	@if [ -d "./out/objs/" ];then rm -r ./out/objs/;fi
	@mkdir -p ./out/Linux
	@mkdir -p ./out/objs/core
	@mkdir -p ./out/objs/core/CrystalCore
	@mkdir -p ./out/objs/core/CrystalAlgorithms
	@mkdir -p ./out/objs/core/CrystalThread
	@mkdir -p ./out/objs/core/CrystalAudio
	@mkdir -p ./out/objs/core/CrystalGraphic
	@mkdir -p ./out/objs/src/CrystalCore
	@mkdir -p ./out/objs/src/CrystalAlgorithms
	@mkdir -p ./out/objs/src/CrystalThread
	@mkdir -p ./out/objs/src/CrystalAudio
	@mkdir -p ./out/objs/src/CrystalGraphic
	@mkdir -p ./out/objs/shader

debug:
	@echo ===========
	@echo $(OBJ_CORE_FILES_CORE)
	@echo $(OBJ_FILES_CORE)
	@echo ===========

./out/objs/shader/%.o : ./shader/%.glsl
	@ld -r -b binary $^ -o $@

./out/objs/%.o : ./%.c
	@gcc -fvisibility=hidden -fPIC -c $^ -o $@ $(INCLUDE_PATH)

core : $(OBJ_MOD_FILES_CORE) $(OBJ_CORE_FILES_CORE)
	@ar rcs out/Linux/libCrystalCore.a $(OBJ_CORE_FILES_CORE)
	@gcc -shared -o out/Linux/CrystalCore.so $(OBJ_MOD_FILES_CORE) $(INCLUDE_PATH) $(LIBRARIES)

algorithms : $(OBJ_MOD_FILES_ALG) $(OBJ_CORE_FILES_ALG)
	@ar rcs out/Linux/libCrystalAlgorithms.a $(OBJ_CORE_FILES_ALG)
	@gcc -shared -o out/Linux/CrystalAlgorithms.so $(OBJ_MOD_FILES_ALG) $(INCLUDE_PATH) $(LIBRARIES)

thread : $(OBJ_MOD_FILES_THR) $(OBJ_CORE_FILES_THR)
	@ar rcs out/Linux/libCrystalThread.a $(OBJ_CORE_FILES_THR)
	@gcc -shared -o out/Linux/CrystalThread.so $(OBJ_MOD_FILES_THR) $(INCLUDE_PATH) $(LIBRARIES)

audio : $(OBJ_MOD_FILES_AUDIO) $(OBJ_CORE_FILES_AUDIO)
	@ar rcs out/Linux/libCrystalAudio.a $(OBJ_CORE_FILES_AUDIO)
	@gcc -shared -o out/Linux/CrystalAudio.so $(OBJ_MOD_FILES_AUDIO) $(INCLUDE_PATH) $(LIBRARIES)

graphic : $(OBJ_MOD_FILES_GRC) $(OBJ_CORE_FILES_GRC) $(OBJ_SHADER_FILES)
	@ar rcs out/Linux/libCrystalGraphic.a $(OBJ_CORE_FILES_GRC)
	@gcc -shared -o out/Linux/CrystalGraphic.so $(OBJ_MOD_FILES_GRC) $(OBJ_SHADER_FILES) $(INCLUDE_PATH) $(LIBRARIES)

build : clean core algorithms thread audio graphic
	@gcc -o out/Linux/app.run $(APP_FILES) $(INCLUDE_PATH) -L./out/Linux $(APP_DEPENDS)

run : build
	@./out/Linux/app.run

app :
	@gcc -o out/Linux/app.run $(APP_FILES) $(INCLUDE_PATH) -L./out/Linux $(APP_DEPENDS)
	@./out/Linux/app.run

.PHONY: debug core algorithms thread audio graphic shader build run app