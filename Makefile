CXX = clang++
CXXFLAGS = -std=c++17 -O2 -Wall -Iinclude

# Flags séparés : includes seuls pour la compilation, libs seules pour le link
OPENCV_CFLAGS = $(shell pkg-config --cflags opencv4)
OPENCV_LIBS = $(shell pkg-config --libs opencv4)

OMP_CFLAGS = -Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include
OMP_LIBS = -L/opt/homebrew/opt/libomp/lib -lomp

SRC_DIR = src
APPS_DIR = apps
BUILD_DIR = build

# Tous les .cpp de src/ (logique métier)
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
SRC_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRC_FILES))

# Tous les .cpp de apps/ (chacun devient un exécutable)
APP_FILES = $(wildcard $(APPS_DIR)/*.cpp)
APPS = $(patsubst $(APPS_DIR)/%.cpp,$(BUILD_DIR)/%,$(APP_FILES))

all: $(APPS)

# Empêche Make de supprimer les .o intermédiaires après le link
.SECONDARY: $(SRC_OBJS)

# Link final : includes + libs
$(BUILD_DIR)/%: $(APPS_DIR)/%.cpp $(SRC_OBJS) | $(BUILD_DIR)
	@echo "Link  $@"
	@$(CXX) $(CXXFLAGS) $(OPENCV_CFLAGS) $(OMP_CFLAGS) $< $(SRC_OBJS) -o $@ $(OPENCV_LIBS) $(OMP_LIBS)

# Compilation des .o : includes seulement, pas de libs
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compile  $<"
	@$(CXX) $(CXXFLAGS) $(OPENCV_CFLAGS) $(OMP_CFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

clean:
	@rm -rf $(BUILD_DIR)

.PHONY: all clean