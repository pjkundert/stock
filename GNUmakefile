
SOURCES		:=	main.C 		\
		 	stockticker.C	\
			stock.C		\
			player.C	\
			holding.C
OBJS		:= $(SOURCES:.C=.o)

CC		:= c++
CXXFLAGS	+= -O3 -I. -I /usr/include/ncurses
LDFLAGS		+= -O3 -lncurses

$(SOURCES):	stock.H

stock:		$(OBJS)

clean:
	rm -f $(OBJS) stock
