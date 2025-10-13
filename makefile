target:
	g++ main.cpp -lreadline -o yatts

clean:
	rm yatts

backup:
	cp tasks.txt tasks2.txt
	cp history.txt history2.txt
