# This file is part of ExtractDRS.

# ExtractDRS is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# ExtractDRS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with ExtractDRS.  If not, see <http://www.gnu.org/licenses/>.

OBJDIR=obj

CXX=g++
CXXFLAGS=-Wall -g

CXXFILES=extractdrs.cpp drs.cpp

OBJFILES=$(addprefix $(OBJDIR)/,$(CXXFILES:.cpp=.o))
TARGET=extractdrs

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJFILES)

run: $(TARGET)
	./$(TARGET)

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	for i in $(OBJFILES) $(DEPEND) $(TARGET);\
	do if test -f $$i; then rm $$i; fi;\
	done

.PHONY: run clean all
