# Makefile for DVS901H
# Copyright(C) 2005-2010 TaiTech Inc.

# Cross Compile Tools
CC 			:= g++

# Macro Of Platform

# Macro Of Header

# Macro Of Function

# Macro Of Library
LDFLAGS	+= -lpthread 

# Object path configure
OBJSPATH 	:= ./onvifobj
OBJS 		:= $(wildcard $(OBJSPATH)/*.o)

# Specific Rules
.PHONY: all

all:
	$(if $(shell find $(OBJSPATH)), $(shell cd ./), $(shell mkdir $(OBJSPATH)))
	$(CC) -c -g soapServer.cpp -o $(OBJSPATH)/soapServer.o $(CFLAGS)
	$(CC) -c -g onvifDiscovery.cpp -o $(OBJSPATH)/onvifDiscovery.o $(CFLAGS)
	$(CC) -c -g onvifService.cpp -o $(OBJSPATH)/onvifService.o $(CFLAGS)
	$(CC) -c -g libonvif.cpp -o $(OBJSPATH)/libonvif.o $(CFLAGS)
	$(CC) -c -g gen_uuid.cpp -o $(OBJSPATH)/gen_uuid.o $(CFLAGS)
	$(CC) -c -g onvif.cpp -o $(OBJSPATH)/onvif.o $(CFLAGS)
	$(CC) -c -g main.cpp -o ./main.o $(CFLAGS)

	$(CC) $(OBJSPATH)/*.o ./main.o $(LDFLAGS) -o onvif
	
clean: 
	rm -f $(OBJSPATH)/*.o $(OBJSPATH)/*.a $(OBJSPATH)/*.out
	rm -rf onvif
	rm -f *~
	rmdir $(OBJSPATH)
