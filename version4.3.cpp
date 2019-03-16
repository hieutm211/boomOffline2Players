
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
	int itemDelay; //seconds
	time_t startTime;
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
	string type;
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
		>> map.maxSpeed >> map.itemDelay;

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

	mapFileName = "map4.1.txt";
	cFileName = "character3.1.txt";
	readMap(mapFileName);
	readCharacter(cFileName);
	
}

void initiate(const string& mapFileName, const string& characterFileName) {
	print("Initiating...");

	player[0].character = 0;
	player[1].character = 0;

	getInitPosition('A', player[0]);
	getInitPosition('B', player[1]);

	for (int i = 0; i < 2; i++) {
		int cId = player[i].character;
		player[i].numberOfBombs = character[cId].numberOfBombs;
		player[i].bombSize = character[cId].bombSize;
		player[i].speed = character[cId].speed;
	}

	map.startTime = time(NULL);

	print("");
}

double bombCurrentDuration(const tBomb& bomb) {
	time_t timeNow = time(NULL);

	return difftime(timeNow, bomb.timeStart);
}

bool bombInProcess(const tBomb& bomb) {
	double currentDuration = bombCurrentDuration(bomb); 	
	return map.bombDuration < currentDuration && currentDuration <= map.bombDuration + map.bombProcessTime;
}

bool bombExploded(const tBomb& bomb) {
	double currentDuration = bombCurrentDuration(bomb);
	return map.bombDuration + map.bombProcessTime < currentDuration;
}

bool isBlock(const int& x, const int& y) {
	return map.matrix[x][y] == '*';
}

bool isConcrete(const int& x, const int& y) {
	return map.matrix[x][y] == '#';
}

bool isEmptyCell(const int x, const int y) {
	return map.matrix[x][y] == '.';
}

void getPosition4Direction(const tBomb& bomb, int& top, int& bottom, int& left, int& right) {
	int bombSize = player[bomb.owner].bombSize;

	top = bomb.x;
	bottom = bomb.x;
	left = bomb.y;
	right = bomb.y;

	for (int i = 0; i < bombSize; i++) {
		if (top-1 >= 0 && isEmptyCell(top, bomb.y)) top--;
		if (bottom+1 < map.row && isEmptyCell(bottom, bomb.y)) bottom++;
		if (left-1 >= 0 && isEmptyCell(bomb.x, left)) left--;
		if (right+1 < map.col && isEmptyCell(bomb.x, right)) right++;
	}
}

bool pointInLine(int x, int L, int R) {
	return L <= x && x <= R;
}

void markConnectedBombs(int k, vector<bool>& mark) {
	if (mark[k] == true) {
		return;
	}

	mark[k] = true;

	int top, bottom, left, right;
	getPosition4Direction(bomb[k], top, bottom, left, right);
	
	for (int i = 0; i < bomb.size(); i++) {
		if (bomb[i].x == bomb[k].x && pointInLine(bomb[i].y, left, right)) markConnectedBombs(i, mark);
		if (bomb[i].y == bomb[k].y && pointInLine(bomb[i].x, top, bottom)) markConnectedBombs(i, mark);
	}
}

void markBombLine(const vector<tBomb>& bomb, vector<vector<char>>& mapBind) {
	vector<bool> markProcessingBomb(bomb.size(), false);

	int x, y;

	for (int i = 0; i < bomb.size(); i++) {
		x = bomb[i].x;
		y = bomb[i].y;

		mapBind[x][y] = (char)('a' + bomb[i].owner);
	}

	for (int i = 0; i < bomb.size(); i++) {
		if (bombInProcess(bomb[i])) {
			markConnectedBombs(i, markProcessingBomb);
		}
	}

	for (int k = 0; k < bomb.size(); k++) {
		if (markProcessingBomb[k] == true) {
			int bombSize = player[bomb[k].owner].bombSize;
			char bombChar = (char) ('0' + bomb[k].owner);
			int top, bottom, left, right;

			x = bomb[k].x;
			y = bomb[k].y;

			getPosition4Direction(bomb[k], top, bottom, left, right);

			for (int i = top; i < bottom+1; i++) {
				mapBind[i][y] = bombChar;
			}
			for (int j = left; j < right+1; j++) {
				mapBind[x][j] = bombChar;
			}
		}
	}
}

void bind(const tMap& map, const vector<tPlayer>& player, const vector<tBomb>& bomb, const vector<tItem>& item, vector<vector<char>>& mapBind) {
	int x, y;

	for (int i = 0; i < player.size(); i++) {
		x = player[i].x;
		y = player[i].y;
		mapBind[x][y] = (char)('A' + i);
	}

	time_t timeNow = time(NULL);

	markBombLine(bomb, mapBind);

	for (int i = 0; i < item.size(); i++) {
		x = item[i].x;
		y = item[i].y;
		
		if (item[i].type == "bomb") {
			mapBind[x][y] = '5';
		} else {  
			if (item[i].type == "bombSize") {
				mapBind[x][y] = '6';		
			}
		}
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

int countBomb(const int& playerId) {
	int ans = 0;
	for (int i = 0; i < bomb.size(); i++) {
		if (bomb[i].owner == playerId) {
			ans++;
		}
	}
	return ans;
}

bool validInput(tEvent event) {
	bool ans = true;

	switch (event.value) {
		case 'd':
			if (!inMap(event.x, event.y)) {
				ans = false;
			}

			if (isBlock(event.x, event.y) || isConcrete(event.x, event.y)) {
				ans = false;
			}

			break;

		case 'b':
			if (existedBomb(event.x, event.y)) {
				ans = false;
			}

			if (countBomb(event.owner) == player[event.owner].numberOfBombs) {
				ans = false;
			}
			
			break;
	}

	return ans;
}

void updateUserCoordinate(const int& id, const int& newX, const int& newY) {
	player[id].x = newX;
	player[id].y = newY;
}

void addABomb(const tEvent& event) {
	bomb.push_back({event.x, event.y, time(NULL), event.owner});	
}

void removeBlocksInBombLine(const vector<tBomb>& bomb, const vector<bool>& markExplodedBomb) {
	vector<vector<bool>> mark(map.row, vector<bool>(map.col, false));

	for (int i = 0; i < bomb.size(); i++) {
		if (markExplodedBomb[i] == true) {
			int top, bottom, left, right;
			int x = bomb[i].x;
			int y = bomb[i].y;

			getPosition4Direction(bomb[i], top, bottom, left, right);
		
			mark[top][y] = true;
			mark[bottom][y] = true;
			mark[x][left] = true;
			mark[x][right] = true;
		}
	}	

	for (int i = 0; i < map.row; i++) {
		for (int j = 0; j < map.col; j++) {
			if (mark[i][j] == true && isBlock(i, j)) {
				map.matrix[i][j] = '.';
			}
		}
	}
}

void removeItemsInBombLine(const vector<tBomb>& bomb, const vector<bool>& markExplodedBomb) {
	vector<vector<bool>> mark(map.row, vector<bool>(map.col, false));

	for (int i = 0; i < bomb.size(); i++) {
		if (markExplodedBomb[i]) {
			int top, bottom, left, right;
			int x = bomb[i].x;
			int y = bomb[i].y;

			getPosition4Direction(bomb[i], top, bottom, left, right);
		
			for (int xx = top; xx < bottom+1; xx++) {
				mark[xx][y] = true;
			}
			for (int yy = left; yy < right+1; yy++) {
				mark[x][yy] = true;
			}
		}
	}	

	for (int i = item.size()-1; i >= 0; i--) {
		if (mark[item[i].x][item[i].y] == true) {
			item.erase(item.begin() + i);
		}
	}
}

void removeExplodedBomb(vector<tBomb>& bomb) {
	vector<bool> markExplodedBomb(bomb.size(), false);

	for (int i = 0; i < bomb.size(); i++) {
		if (bombExploded(bomb[i])) {
			markConnectedBombs(i, markExplodedBomb);
		}
	} 
	
	removeItemsInBombLine(bomb, markExplodedBomb);
	removeBlocksInBombLine(bomb, markExplodedBomb);

	for (int i = bomb.size()-1; i >= 0; i--) {
		if (markExplodedBomb[i]) {
			bomb.erase(bomb.begin() + i);
		}
	}

}

void setItem(const tEvent& event) {
	switch (event.value) {
		case '5': 
			item.push_back({event.x, event.y, "bomb"});
			break;
		case '6':
			item.push_back({event.x, event.y, "bombSize"});
			break;
	}
}

void processItem(const int& owner, const int& x, const int& y) {
	for (int i = 0; i < item.size(); i++) {
		if (item[i].x == x && item[i].y == y) {
			if (item[i].type == "bomb") {
				if (player[owner].numberOfBombs < map.maxBomb) {
					player[owner].numberOfBombs++;
				}
			} else {
				if (item[i].type == "bombSize") {
					if (player[owner].bombSize < map.maxBombSize) {
						player[owner].bombSize++;		
					}
				}

			}
			item.erase(item.begin()+i);
			break;
		}
	}	
}

void updateGameStatus(const tEvent& event) {
	switch (event.value) {
		case 'd':
			updateUserCoordinate(event.owner, event.x, event.y);		
			processItem(event.owner, event.x, event.y);
			break;
		case 'b':
			addABomb(event);
			break;
		case '5':
			setItem(event);	
			break;
		case '6': 
			setItem(event);
			break;
	}

	removeExplodedBomb(bomb);
}

bool inBombLine(const int& x, const int& y) {
	int bombSize;

	for (int i = 0; i < bomb.size(); i++) {
		if (bombInProcess(bomb[i])) {
			bombSize = player[bomb[i].owner].bombSize;

			int top, bottom, left, right;
			getPosition4Direction(bomb[i], top, bottom, left, right);
			if (y == bomb[i].y && pointInLine(x, top, bottom)) return true;
			if (x == bomb[i].x && pointInLine(y, left, right)) return true;
		}
	}

	return false;
}


int stopCondition() {
	bool player0dead = inBombLine(player[0].x, player[0].y);
	bool player1dead = inBombLine(player[1].x, player[1].y);

	if (player0dead == true && player1dead == true) {
		return 2;
	}

	if (player1dead == true) {
		return 0;
	}

	if (player0dead == true) {
		return 1;
	}

	return -1;	
}

bool addAnEvent() {
	return ((int) difftime(time(NULL), map.startTime) % map.itemDelay == 0); 
}
void endGame(const int& stopCode) {
	switch(stopCode) {
		case 0: 
			cout << "Player 0 win" << endl;
			break;
		case 1: 
			cout << "Player 1 win" << endl;
			break;
		default: 
			cout << "Draw!" << endl;
			break;
	}
}

void generateAnItem(tEvent& event) {
	vector<pair<int, int>> emptyCell;

	for (int i = 0; i < map.row; i++) {
		for (int j = 0; j < map.col; j++) {
			if (map.matrix[i][j] == '.') {
				emptyCell.push_back({i, j});
			}
		}	
	}

	for (int i = emptyCell.size(); i >= 0; i--) {
		for (int j = 0; j < item.size(); j++) {
			if (emptyCell[i].first == item[j].x && emptyCell[i].second == item[j].y) {
				emptyCell.erase(emptyCell.begin()+i);
				break;
			}
		}
	}

	srand(time(NULL));

	int itemType;	
	pair<int, int> itemPosition;

	if (rand()%100 < 50) {
		itemType = 5; //increase bomb	
	} else {
		itemType = 6; //increase bomb size  
	}

	itemPosition = emptyCell[rand() % emptyCell.size()];	

	event = {(char) ('0' + itemType), itemPosition.first, itemPosition.second, 0};
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

		int stopCode = stopCondition();
		if (stopCode != -1) {
			endGame(stopCode);
			break;
		}

		if (addAnEvent()) {
			tEvent event;
			generateAnItem(event);
			updateGameStatus(event);
		}
	}
}

int main() {
	start();
	initiate(mapFileName, cFileName);
	gameLoop();	
	return 0;
}
