### To compile and run with g++
g++-14 -std=c++23 -I. -o xml_search_fields main.cpp pugixml.cpp -fopenmp -pthread
### To create XMLs for program use python script generator.py
The python script defaultly creates 400 000 example XMLs, you can edit this number in the code. (±400 000 recommended for testing)
