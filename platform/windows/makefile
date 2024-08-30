INCLUDE_PATH := .\include
INCLUDE_PATH += .\shader\include
INCLUDE_PATH += .\platform\Vkinclude

LIBRARIES := winmm
LIBRARIES += opengl32
LIBRARIES += gdi32
LIBRARIES += ole32

APP_DEPENDS := CrystalCore
APP_DEPENDS += CrystalAlgorithms
APP_DEPENDS += CrystalThread
APP_DEPENDS += CrystalAudio
APP_DEPENDS += CrystalGraphic

INCLUDE_PATH := $(foreach item,$(INCLUDE_PATH),-I$(item))

LIBRARIES := $(foreach item,$(LIBRARIES),-l$(item))
LIBRARIES += .\platform\windows\vulkan-1.lib

APP_DEPENDS := $(foreach item,$(APP_DEPENDS),-l$(item))

C_FILES := $(shell powershell Get-ChildItem -Recurse -Filter "*.c" -Path . -Name)
APP_FILES := $(filter application\\%.c,$(C_FILES))
C_FILES_MOD := $(filter src\\%.c,$(C_FILES))
C_FILES_CORE := $(filter core\\%.c,$(C_FILES))
OBJ_FILES_MOD := $(patsubst src\\%.c,out\objs\src\\%.o,$(C_FILES_MOD))
OBJ_FILES_CORE := $(patsubst core\\%.c,out\objs\core\\%.o,$(C_FILES_CORE))

SHADER_FILES := $(shell powershell Get-ChildItem -Recurse -Filter "*.glsl" -Path . -Name)
OBJ_SHADER_FILES := $(patsubst %.glsl,out\objs\\%.o,$(SHADER_FILES))

VERTSHADER_FILES := $(shell powershell Get-ChildItem -Recurse -Filter "*.vert" -Path . -Name)
VERTSPV_SHADER_FILES := $(patsubst %.vert,out\objs\\%.vert.spv,$(VERTSHADER_FILES))
VERTOBJ_SHADER_FILES := $(patsubst %.spv,%.ov,$(VERTSPV_SHADER_FILES))

FRAGSHADER_FILES := $(shell powershell Get-ChildItem -Recurse -Filter "*.frag" -Path . -Name)
FRAGSPV_SHADER_FILES := $(patsubst %.frag,out\objs\\%.frag.spv,$(FRAGSHADER_FILES))
FRAGOBJ_SHADER_FILES := $(patsubst %.spv,%.of,$(FRAGSPV_SHADER_FILES))

C_MOD_FILES_CORE := $(filter src\CrystalCore\\%.c,$(C_FILES_MOD))
C_MOD_FILES_ALG := $(filter src\CrystalAlgorithms\\%.c,$(C_FILES_MOD))
C_MOD_FILES_THR := $(filter src\CrystalThread\\%.c,$(C_FILES_MOD))
C_MOD_FILES_AUDIO := $(filter src\CrystalAudio\\%.c,$(C_FILES_MOD))
C_MOD_FILES_GRC := $(filter src\CrystalGraphic\\%.c,$(C_FILES_MOD))
OBJ_MOD_FILES_CORE := $(filter out\objs\src\CrystalCore\\%.o,$(OBJ_FILES_MOD))
OBJ_MOD_FILES_ALG := $(filter out\objs\src\CrystalAlgorithms\\%.o,$(OBJ_FILES_MOD))
OBJ_MOD_FILES_THR := $(filter out\objs\src\CrystalThread\\%.o,$(OBJ_FILES_MOD))
OBJ_MOD_FILES_AUDIO := $(filter out\objs\src\CrystalAudio\\%.o,$(OBJ_FILES_MOD))
OBJ_MOD_FILES_GRC := $(filter out\objs\src\CrystalGraphic\\%.o,$(OBJ_FILES_MOD))

C_CORE_FILES_CORE := $(filter core\CrystalCore\\%.c,$(C_FILES_CORE))
C_CORE_FILES_ALG := $(filter core\CrystalAlgorithms\\%.c,$(C_FILES_CORE))
C_CORE_FILES_THR := $(filter core\CrystalThread\\%.c,$(C_FILES_CORE))
C_CORE_FILES_AUDIO := $(filter core\CrystalAudio\\%.c,$(C_FILES_CORE))
C_CORE_FILES_GRC := $(filter core\CrystalGraphic\\%.c,$(C_FILES_CORE))
OBJ_CORE_FILES_CORE := $(filter out\objs\core\CrystalCore\\%.o,$(OBJ_FILES_CORE))
OBJ_CORE_FILES_ALG := $(filter out\objs\core\CrystalAlgorithms\\%.o,$(OBJ_FILES_CORE))
OBJ_CORE_FILES_THR := $(filter out\objs\core\CrystalThread\\%.o,$(OBJ_FILES_CORE))
OBJ_CORE_FILES_AUDIO := $(filter out\objs\core\CrystalAudio\\%.o,$(OBJ_FILES_CORE))
OBJ_CORE_FILES_GRC := $(filter out\objs\core\CrystalGraphic\\%.o,$(OBJ_FILES_CORE))

clean:
	@if not exist out\ (mkdir .\out)
	@if exist out\Windows\ (rmdir /s/q .\out\Windows)
	@if exist out\objs\ (rmdir /s/q .\out\objs)
	@mkdir .\out\Windows
	@mkdir .\out\objs\core
	@mkdir .\out\objs\core\CrystalCore
	@mkdir .\out\objs\core\CrystalAlgorithms
	@mkdir .\out\objs\core\CrystalThread
	@mkdir .\out\objs\core\CrystalAudio
	@mkdir .\out\objs\core\CrystalGraphic
	@mkdir .\out\objs\src\CrystalCore
	@mkdir .\out\objs\src\CrystalAlgorithms
	@mkdir .\out\objs\src\CrystalThread
	@mkdir .\out\objs\src\CrystalAudio
	@mkdir .\out\objs\src\CrystalGraphic
	@mkdir .\out\objs\shader

debug:
	@echo ===========
	@echo $(VERTSPV_SHADER_FILES)
	@echo $(VERTOBJ_SHADER_FILES)
	@echo ===========

out\objs\shader\\%.vert.spv : shader\\%.vert
	.\platform\windows\glslc.exe $^ -o $@

out\objs\shader\\%.frag.spv : shader\\%.frag
	.\platform\windows\glslc.exe $^ -o $@

out\objs\shader\\%.vert.ov : out\objs\shader\\%.vert.spv
	@ld -r -b binary $^ -o $@

out\objs\shader\\%.frag.of : out\objs\shader\\%.frag.spv
	@ld -r -b binary $^ -o $@

out\objs\shader\\%.o : shader\\%.glsl
	@ld -r -b binary $^ -o $@

out\objs\\%.o : %.c
	@gcc -fvisibility=hidden -fPIC -c $^ -o $@ $(INCLUDE_PATH)

core : $(OBJ_MOD_FILES_CORE) $(OBJ_CORE_FILES_CORE)
	@ar rcs out/Windows/libCrystalCore.a $(OBJ_CORE_FILES_CORE)
	@gcc -shared -o out/Windows/CrystalCore.so $(OBJ_MOD_FILES_CORE) $(INCLUDE_PATH) $(LIBRARIES)

algorithms : $(OBJ_MOD_FILES_ALG) $(OBJ_CORE_FILES_ALG)
	@ar rcs out/Windows/libCrystalAlgorithms.a $(OBJ_CORE_FILES_ALG)
	@gcc -shared -o out/Windows/CrystalAlgorithms.so $(OBJ_MOD_FILES_ALG) $(INCLUDE_PATH) $(LIBRARIES)

thread : $(OBJ_MOD_FILES_THR) $(OBJ_CORE_FILES_THR)
	@ar rcs out/Windows/libCrystalThread.a $(OBJ_CORE_FILES_THR)
	@gcc -shared -o out/Windows/CrystalThread.so $(OBJ_MOD_FILES_THR) $(INCLUDE_PATH) $(LIBRARIES)

audio : $(OBJ_MOD_FILES_AUDIO) $(OBJ_CORE_FILES_AUDIO)
	@ar rcs out/Windows/libCrystalAudio.a $(OBJ_CORE_FILES_AUDIO)
	@gcc -shared -o out/Windows/CrystalAudio.so $(OBJ_MOD_FILES_AUDIO) $(INCLUDE_PATH) $(LIBRARIES)

graphic : $(OBJ_MOD_FILES_GRC) $(OBJ_CORE_FILES_GRC) $(OBJ_SHADER_FILES) $(VERTOBJ_SHADER_FILES) $(FRAGOBJ_SHADER_FILES)
	@ar rcs out/Windows/libCrystalGraphic.a $(OBJ_CORE_FILES_GRC)
	@gcc -shared -o out/Windows/CrystalGraphic.so $(OBJ_MOD_FILES_GRC) $(OBJ_SHADER_FILES) $(VERTOBJ_SHADER_FILES) $(FRAGOBJ_SHADER_FILES) $(INCLUDE_PATH) $(LIBRARIES)

build : clean core algorithms thread audio graphic
	@gcc -o out/Windows/app.exe $(APP_FILES) $(INCLUDE_PATH) -L.\out\Windows $(APP_DEPENDS)

run : build
	@.\out\Windows\app.exe

.PHONY: debug core algorithms thread audio graphic shader build run