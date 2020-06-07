HANDIN_FILE=handin.tar.gz
TARGET_SG = start_gap_simulation
TARGET_SG_SANITIZE = start_gap_simulation_sanitize
TARGET_CONFIG = generate_mapping_table_config_file
TARGET_NEW_SG = new_start_gap_simulation
OBJS_CONFIG += generate_mapping_table_config_file.o
OBJS_NEW_SG += new_start_gap_simulation.o
OBJS_SG += start_gap_simulation.o 
OBJS_SG += utils.o
OBJS_SG += sha256.o
LIBS += -lm
LIBS += -lpthread
#LIBS += -lboost_fiber
LIBS += -lboost_system
LIBS += -lboost_thread
#LIBS += -lstdc++

CC = g++
CFLAGS += -MMD -MP # dependency tracking flags
CFLAGS += -I./
CFLAGS += -Wall 
CFLAGS += -std=c++11
#CFLAGS += -Werror 
CFLAGS += -Wconversion
CFLAGS += -Wmaybe-uninitialized
#CFLAGS += -fPIC
#CFLAGS += -m64
#LDFLAGS += -lstdc++ 
LDFLAGS += $(LIBS)

all: CFLAGS += -g -O3 # release flags
all: $(TARGET_SG) $(TARGET_NO) $(TARGET_CONFIG) $(TARGET_NEW_SG) $(TARGET_SG_SANITIZE)

release: clean all

debug: CFLAGS += -ggdb -O0 -D_GLIBC_DEBUG # debug flags
debug: clean $(TARGET_SG) $(TARGET_NO) $(TARGET_NEW_SG) $(TARGET_CONFIG) $(TARGET_SG_SANITIZE)

$(TARGET_NEW_SG): $(OBJS_NEW_SG)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_SG): $(OBJS_SG)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_NO): $(OBJS_NO)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_CONFIG): $(OBJS_CONFIG)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_SG_SANITIZE): $(OBJS_SG)
	$(CC) $(CFLAGS) -fsanitize=thread -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

DEPS = $(OBJS:%.o=%.d)
-include $(DEPS)

clean:
	-@rm *.d *.o $(TARGET_SG) $(TARGET_NEW_SG) $(TARGET_NO) $(TARGET_CONFIG) $(TARGET_SG_SANITIZE) $(OBJS_SG) $(OBJS_NO) $(DEPS) $(OBJS_*)valgrind.log $(HANDIN_FILE) 2> /dev/null || true

test: release
	@./b

handin: clean
	@tar cvzf $(HANDIN_FILE) *
	@echo "***"
	@echo "*** SUCCESS: Created $(HANDIN_FILE)"
	@echo "***"
