compile:
	mkdir -p build
	g++ -o build/cppwiz cppwiz.cpp
run: compile
	./build/cppwiz 192.168.0.100
clean:
	rm -rf build