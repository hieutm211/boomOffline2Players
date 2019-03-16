
#include <iostream>
#include <vector>
#include <fstream>
using namespace std;

const char cDirect[2][4] = {
	{'w', 's', 'a', 'd'}, 
	{'i', 'k', 'j', 'l'}
};

const int directX[4] = {-1, +1, -0, +0};
const int directY[4] = {-0, +0, -1, +1};

const char cBomb[2] = {'z', '/'};

struct tMap {
	int row;
	int col;
	vector< vector<char> > matrix;
	int maxBomb;
	int maxBombSize;
	int maxSpeed;
	double bombDuration;
	double bombProcessTime;
	double gameDuration;
};

struct tPlayer {
	int x;
	int y;
	string name;
	int character;
	int numberOfBombs;
	int bombSize;
	int speed;
};

struct tBomb {
	int x;
	int y;
	time_t timeStart;
	int owner;
};

struct tItem {
	int x;
	int y;
	int type;
};

struct tCharacter {
	string name;
	int numberOfBombs;
	int bombSize;
	int speed;
};

struct tEvent {
	char value;
	int x;
	int y;
	int owner;
};

string mapFileName;
string cFileName;

tMap map;
vector<tPlayer> player(2);
vector<tBomb> bomb;
vector<tItem> item;
vector<tCharacter> character;

void print(string s) {
	cout << s << endl;
}

template<class T>
void print2dVec(vector<vector<T>>& arr) {
	for (int i = 0; i < arr.size(); i++) {
		for (int j = 0; j < arr[i].size(); j++) {
			cout << arr[i][j] << " ";
		}
		cout << endl;
	}
	cout << endl;
}

void readMap(const string& mapFileName) {
	print("reading map: begin");

	ifstream mapFile;

	mapFile.open(mapFileName, ifstream::in);
	if (!mapFile.is_open()) {
		cout << "Cannot open mapFile " << mapFileName << endl;
		return;
	}
	
	mapFile >> map.row >> map.col >> map.gameDuration 
		>> map.maxBomb >> map.maxBombSize >> map.bombDuration >> map.bombProcessTime
		>> map.maxSpeed;

	for (int i = 0; i < map.row; i++) {
		map.matrix.push_back(vector<char>(map.col));
		for (int j = 0; j < map.col; j++) {
			mapFile >> map.matrix[i][j];
		}
	}

	mapFile.close();

	print("reading map: done.\n");
}

void readCharacter(const string& cFileName) {
	print("reading character: begin");

	ifstream cFile;
	cFile.open(cFileName, ifstream::in);
	if (!cFile.is_open()) {
		cout << "Cannot open cFile " << cFileName << endl;
		return;
	}

	int numberOfCharacter;
	cFile >> numberOfCharacter;

	character.resize(numberOfCharacter);

	for (int i = 0; i < character.size(); i++) {
		cFile >> character[i].name >> character[i].numberOfBombs >> character[i].bombSize >> character[i].speed;
	}

	cFile.close();

	print("reading character: done.\n");
} 

void getInitPosition(char c, tPlayer& p) {
	for (int i = 0; i < map.row; i++) {
		for (int j = 0; j < map.col; j++) {
			if (map.matrix[i][j] == c) {
				p.x = i;
				p.y = j;
				map.matrix[i][j] = '.';
				return;
			}
		}
	}	
}

void start() {
	print("starting...");

	mapFileName = "map.txt";
	cFileName = "character.txt";
	readMap(mapFileName);
	readCharacter(cFileName);
	
}

void initiate(const string& mapFileName, const string& characterFileName) {
	print("Initiating...");

	player[0].character = 0;
	player[1].character = 0;

	getInitPosition('A', player[0]);
	getInitPosition('B', player[1]);

	print("");
}

void bind(const tMap& map, const vector<tPlayer>& player, const vector<tBomb>& bomb, const vector<tItem>& item, vector<vector<char>>& mapBind) {
	int x, y;

	for (int i = 0; i < player.size(); i++) {
		x = player[i].x;
		y = player[i].y;
		mapBind[x][y] = (char)('A' + i);
	}

	for (int i = 0; i < bomb.size(); i++) {
		x = bomb[i].x;
		y = bomb[i].y;
		mapBind[x][y] = (char)('a' + bomb[i].owner);
	}
}

void render() {
	system("clear");

	vector<vector<char>> mapBind(map.matrix.begin(), map.matrix.end());
	bind(map, player, bomb, item, mapBind);

	for (int i = 0; i < map.row; i++) {
		for (int j = 0; j < map.col; j++) {
			cout << mapBind[i][j] << " ";
		}	
		cout << endl;
	}
}

bool checkKeystroke(char keystroke, int id) {
	if (keystroke == cBomb[id]) return true;
	for (int i = 0; i < 4; i++) {
		if (keystroke == cDirect[id][i]) return true;
	}	
	return false;
}

tEvent getUserInput() {
	tEvent event = {'.', -1, -1, -1};
	cin >> event.value;

	for (int i = 0; i < 2; i++) {
		if (checkKeystroke(event.value, i)) {
			event.x = player[i].x;
			event.y = player[i].y;
			event.owner = i;
			return event;
		}
	}
}

bool inMap(const int& x, const int& y) {
	return 0 <= x && x < map.row && 0 <= y && y < map.col;
}

void updateEventInfo(tEvent& event) {
	//item event
	if (event.owner == 2) {
		return;
	}

	//direct event
	for (int i = 0; i < 4; i++) {
		if (event.value == cDirect[event.owner][i]) {
			event.value = 'd';
			event.x += directX[i];
			event.y += directY[i];
			return;
		}
	}
	
	//bomb event
	for (int i = 0; i < 2; i++) {
		if (event.value == cBomb[event.owner]) {
			event.value = 'b';
		}
	}
}

bool existedBomb(const int& x, const int& y) {
	for (int i = 0; i < bomb.size(); i++) {
		if (bomb[i].x == x && bomb[i].y == y) {
			return true;
		}
	}
	return false;
}

bool validInput(tEvent event) {
	switch (event.value) {
		case 'd':
			return inMap(event.x, event.y);
		case 'b':
			return !existedBomb(event.x, event.y);
	}
}

void updateUserCoordinate(const int& id, const int& newX, const int& newY) {
	player[id].x = newX;
	player[id].y = newY;
}

void addABomb(const tEvent& event) {
	bomb.push_back({event.x, event.y, time(NULL), event.owner});	
}

void updateGameStatus(const tEvent& event) {
	switch (event.value) {
		case 'd':
			updateUserCoordinate(event.owner, event.x, event.y);		
			break;
		case 'b':
			addABomb(event);
			break;
	}
}
bool stopCondition() {
	return false;	
}

bool addAnEvent() {
	
}
void endGame() {

}

void gameLoop() {
	cout << "Game begin." << endl;	

	while(true) {
		render();
		
		tEvent inputEvent;
		do {
			inputEvent = getUserInput();
			updateEventInfo(inputEvent);
		} while (!validInput(inputEvent));
		
		updateGameStatus(inputEvent);

		if (stopCondition()) {
			endGame();
			break;
		}

		if (addAnEvent()) {
			//updateGameStatus();
		}
	}
}

int main() {
	start();
	initiate(mapFileName, cFileName);
	gameLoop();	
	return 0;
}
