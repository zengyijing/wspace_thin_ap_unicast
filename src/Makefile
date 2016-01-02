#
# To compile, type "make" 
# To remove files, type "make clean"
#
LIBS = -lpthread 
all: wspace_thin_ap_unicast

wspace_thin_ap_unicast: wspace_asym_util.o tun.o time_util.o
	$(CXX) $(CXXFLAGS) $^ -o wspace_thin_ap_unicast $(LIBS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	rm -rf wspace_thin_ap_unicast *.o 

tag: 
	ctags -R *
