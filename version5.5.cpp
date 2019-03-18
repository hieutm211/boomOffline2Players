
#include <SFML/Graphics.hpp>
#include <cstdio>
#include <iostream>
#include <vector>
#include <fstream>
using namespace std;

const int cellHeight = 30;
const int cellWidth = 30;

const string directKey[2][4] = {
	{"W", "S", "A", "D"}, 
	{"Up", "Down", "Left", "Right"}
};

const string bombKey[2] = {"LShift", "RShift"};

const int directX[4] = {-1, +1, -0, +0};
const int directY[4] = {-0, +0, -1, +1};

struct tMap {
	string name;
	int row;
	int col;
	vector< vector<string> > matrix;
	int maxBomb;
	int maxBombSize;
	int maxSpeed;
	int maxItem;
	int gameDuration;
	double bombDuration;
	double bombProcessTime;
	int itemDelay; //seconds
	time_t startTime;
	int stopCode;
};

struct tEntity {
	string symbol;
	bool canGoThrough;
	bool canBreak;
	string imgFile;
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
	int size;
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
	string symbol;
	int numberOfBombs;
	int bombSize;
	int speed;
};

struct tEvent {
	string type;
	string value;
	int x;
	int y;
	int owner;
};

struct tItemList {
	string symbol;
	vector<string> attribute;
};

sf::RenderWindow app;

string mapFileName;
string cFileName;
string itemFileName;

tMap mapData;
vector<tEntity> entity;
vector<tPlayer> player(2);
vector<tBomb> bomb;
vector<tItem> item;
vector<tCharacter> character;
vector<tItemList> itemList;

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
	print("Read mapData: begin");

	ifstream mapFile;
	mapFile.open(mapFileName);
	if (!mapFile.is_open()) {
		cout << "cannot open file " << mapFileName << endl;
		return;
	}

	//start reading from mapFile....................
	string tmp;
	mapFile >> tmp >> tmp >> mapData.name;
	mapFile >> tmp >> tmp >> tmp >> tmp >> mapData.row >> mapData.col >> mapData.gameDuration;
	mapFile >> tmp >> tmp >> tmp >> tmp >> mapData.bombDuration >> mapData.bombProcessTime >> mapData.itemDelay;
	mapFile >> tmp >> tmp >> mapData.maxBomb;
	mapFile >> tmp >> tmp >> mapData.maxBombSize;
	mapFile >> tmp >> tmp >> mapData.maxItem;
	mapFile >> tmp >> tmp >> mapData.maxSpeed;

	for (int i = 0; i < mapData.row; i++) {
		mapData.matrix.push_back(vector<string>(mapData.col));
		for (int j = 0; j < mapData.col; j++) {
			mapFile >> mapData.matrix[i][j];
		}
	}

	int numberOfEntities;
	mapFile >> tmp >> tmp >> numberOfEntities;

	for (int i = 0; i < 5; i++) {
		getline(mapFile, tmp);
	}

	for (int i = 0; i < numberOfEntities; i++) {
		string symbol, imgFile;
		bool canGoThrough, canBreak;
		mapFile >> symbol >> canGoThrough >> canBreak >> imgFile;
		entity.push_back({symbol, canGoThrough, canBreak, imgFile});
	}
	//End Read.................................
	
	mapFile.close();
	
	mapData.stopCode = -1;

	print("Read mapData: done.\n");
}

void readCharacter(const string& cFileName) {
	print("reading character: begin");

	ifstream cFile;
	cFile.open(cFileName, ifstream::in);
	if (!cFile.is_open()) {
		cout << "Cannot open cFile " << cFileName << endl;
		return;
	}

	string tmp;
	int numberOfCharacters;
	cFile >> tmp >> tmp >> numberOfCharacters;

	character.resize(numberOfCharacters);

	for (int i = 0; i < 3; i++) {
		getline(cFile, tmp);
	}

	for (int i = 0; i < character.size(); i++) {
		string bombSymbol, bombLineSymbol;
		string imgFile, bombImgFile, bombLineImgFile;

		//read character
		cFile >> character[i].name >> character[i].numberOfBombs >> character[i].bombSize >> character[i].speed >> character[i].symbol;

		//read entity's information
		cFile >> bombSymbol >> bombLineSymbol >> imgFile >> bombImgFile >> bombLineImgFile;
	
		//add to entity list
		entity.push_back({character[i].symbol, 0, 1, imgFile});
		entity.push_back({bombSymbol, 0, 1, bombImgFile});
		entity.push_back({bombLineSymbol, 1, 0, bombLineImgFile});
	}

	cFile.close();

	print("reading character: done.\n");
} 

void readItem(string itemFileName) {
	print("read items: begin");

	ifstream iFile;
	iFile.open(itemFileName, ifstream::in);
	if (!iFile.is_open()) {
		cout << "Cannot open iFile " << itemFileName << endl;
		return;
	}

	string tmp;
	int numberOfItems;

	iFile >> tmp >> tmp >> numberOfItems;

	itemList.resize(numberOfItems);

	for (int i = 0; i < 3; i++) {
		getline(iFile, tmp);
	}

	for (int i = 0; i < numberOfItems; i++) {
		string imgFile, attr;
		int numberOfAttributes;
		
		itemList[i].attribute.clear();

		//read entity's information
		iFile >> itemList[i].symbol >> imgFile;
		
		//read item
		iFile >> numberOfAttributes >> tmp;

		for (int j = 0; j < numberOfAttributes; j++) {
			iFile >> attr;
			itemList[i].attribute.push_back(attr);
		}

		//add to entity list
		entity.push_back({itemList[i].symbol, 1, 1, imgFile});
	}

	iFile.close();

	print("read items: done.\n");
}

void getInitPosition(string symbol, tPlayer& p) {
	for (int i = 0; i < mapData.row; i++) {
		for (int j = 0; j < mapData.col; j++) {
			if (mapData.matrix[i][j] == symbol) {
				p.x = i;
				p.y = j;
				mapData.matrix[i][j] = '.';
				return;
			}
		}
	}	
}

void start() {
	print("starting...");

	mapFileName = "defaultMap.txt";
	cFileName = "defaultCharacter.txt";
	itemFileName = "defaultItem.txt";

	readMap(mapFileName);
	readCharacter(cFileName);
	readItem(itemFileName);	
}

void initiate() {
	print("Initiating...");

	player[0].character = 0;
	player[1].character = 1;

	for (int i = 0; i < 2; i++) {
		getInitPosition(character[player[i].character].symbol, player[i]);
	}

	for (int i = 0; i < 2; i++) {
		int cId = player[i].character;
		player[i].numberOfBombs = character[cId].numberOfBombs;
		player[i].bombSize = character[cId].bombSize;
		player[i].speed = character[cId].speed;
	}

	mapData.startTime = time(NULL);

	print("");
}

double bombCurrentDuration(const tBomb& bomb) {
	time_t timeNow = time(NULL);

	return difftime(timeNow, bomb.timeStart);
}

bool bombInProcess(const tBomb& bomb) {
	double currentDuration = bombCurrentDuration(bomb); 	
	return mapData.bombDuration < currentDuration && currentDuration <= mapData.bombDuration + mapData.bombProcessTime;
}

bool bombExploded(const tBomb& bomb) {
	double currentDuration = bombCurrentDuration(bomb);
	return mapData.bombDuration + mapData.bombProcessTime < currentDuration;
}

bool canGo(const int x, const int y) {
	for (int i = 0; i < bomb.size(); i++) {
		if (x == bomb[i].x && y == bomb[i].y) {
			return false;
		}
	}
	for (int i = 0; i < entity.size(); i++) {
		if (entity[i].symbol == mapData.matrix[x][y]) {
			return entity[i].canGoThrough;
		}
	}
	return false;
}

bool canBreak(const int x, const int y) {
	for (int i = 0; i < entity.size(); i++) {
		if (entity[i].symbol == mapData.matrix[x][y]) {
			return entity[i].canBreak;
		}
	}
}

void getPosition4Direction(const tBomb& bomb, int& top, int& bottom, int& left, int& right) {
	top = bomb.x-1;
	bottom = bomb.x+1;
	left = bomb.y-1;
	right = bomb.y+1;

	for (int i = 0; i < bomb.size-1; i++) {
		if (top-1 >= 0 && canGo(top, bomb.y)) top--;
		if (bottom+1 < mapData.row && canGo(bottom, bomb.y)) bottom++;
		if (left-1 >= 0 && canGo(bomb.x, left)) left--;
		if (right+1 < mapData.col && canGo(bomb.x, right)) right++;
	}

	top = max(0, top);
	left = max(0, left);
	bottom = min(mapData.row-1, bottom);
	right = min(mapData.col-1, right);
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

void fillBomb(const vector<tBomb>& bom, vector<vector<string>>& mapBind) {
	int x, y;

	for (int i = 0; i < bomb.size(); i++) {
		x = bomb[i].x;
		y = bomb[i].y;

		mapBind[x][y] = (char)('a' + bomb[i].owner);
	}
}

void fillBombLine(const vector<tBomb>& bomb, vector<vector<string>>& mapBind) {
	vector<bool> markProcessingBomb(bomb.size(), false);

	for (int i = 0; i < bomb.size(); i++) {
		if (bombInProcess(bomb[i])) {
			markConnectedBombs(i, markProcessingBomb);
		}
	}

	for (int k = 0; k < bomb.size(); k++) {
		if (markProcessingBomb[k] == true) {
			char bombChar = (char) ('0' + bomb[k].owner);
			int top, bottom, left, right;

			int x = bomb[k].x;
			int y = bomb[k].y;

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

void fillItem(const vector<tItem>& item, vector<vector<string>>& mapBind) {
	for (int i = 0; i < item.size(); i++) {
		int x = item[i].x;
		int y = item[i].y;
		
		mapBind[x][y] = itemList[item[i].type].symbol;
	}
}

void fillPlayer(const vector<tPlayer>& player, vector<vector<string>>& mapBind) {
	for (int i = 0; i < player.size(); i++) {
		int x = player[i].x;
		int y = player[i].y;
		mapBind[x][y] = character[player[i].character].symbol;
	}
}

void fillEntity(const vector<vector<string>>& matrix, vector<vector<string>>& mapBind) {
	for (int i = 0; i < mapData.row; i++) {
		for (int j = 0; j < mapData.col; j++) {
			if (matrix[i][j] != entity[0].symbol) {
				mapBind[i][j] = matrix[i][j];
			}
		}
	}
}

void fillGround(vector<vector<string>>& mapBind) {
	mapBind.resize(mapData.row);

	for (int i = 0; i < mapData.row; i++) {
		mapBind[i].resize(mapData.col);
		
		for (int j = 0; j < mapData.col; j++) {
			mapBind[i][j] = entity[0].symbol;
		}
	}
}

void bind(const tMap& mapData, const vector<tPlayer>& player, const vector<tBomb>& bomb, const vector<tItem>& item, vector<vector<string>>& mapBind) {
	fillGround(mapBind);

	fillItem(item, mapBind);
	
	fillBomb(bomb, mapBind);
	fillPlayer(player, mapBind);

	fillEntity(mapData.matrix, mapBind);

	fillBombLine(bomb, mapBind);
}

void render() {
	system("clear");

	//print time left
	cout << "Time left: " << (int) (mapData.gameDuration - difftime(time(NULL), mapData.startTime)) << " seconds" << endl << endl;

	vector<vector<string>> mapBind;
	bind(mapData, player, bomb, item, mapBind);

	for (int i = 0; i < mapData.row; i++) {
		for (int j = 0; j < mapData.col; j++) {
			cout << mapBind[i][j] << " ";
		}	
		cout << endl;
	}
	cout << endl;

	system("sleep 0.1");
}

bool checkKeystroke(string key, int id) {
	if (key == bombKey[id]) return true;
	for (int i = 0; i < 4; i++) {
		if (key == directKey[id][i]) return true;
	}	
	return false;
}

string getKeyPress(const sf::Event& event) {
	if (event.type != sf::Event::KeyPressed) return "null";

	switch (event.key.code) {
		case sf::Keyboard::W : return "W";
		case sf::Keyboard::S : return "S";
		case sf::Keyboard::A : return "A";
		case sf::Keyboard::D : return "D";
	
		case sf::Keyboard::LShift : return "LShift";

		case sf::Keyboard::Up : return "Up";
		case sf::Keyboard::Down : return "Down";
		case sf::Keyboard::Left : return "Left";
		case sf::Keyboard::Right : return "Right";

		case sf::Keyboard::RShift : return "RShift";

		default:
			return "null";
	}
}

void checkBasicEvent(const sf::Event& event) {
	if (event.type == sf::Event::Closed) {
		app.close();
	}
}

tEvent getUserInput() {
	tEvent event = {"?", "?", -1, -1, -1};
	sf::Event sfEvent;

	do {
		app.waitEvent(sfEvent);
		
		checkBasicEvent(sfEvent);

		event.value = getKeyPress(sfEvent);

		if (mapData.stopCode != -1) return event; 

	} while (event.value == "null");

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
	return 0 <= x && x < mapData.row && 0 <= y && y < mapData.col;
}

void updateEventInfo(tEvent& event) {
	//item event
	if (event.owner == 2) {
		return;
	}

	//direct event
	for (int i = 0; i < 4; i++) {
		if (event.value == directKey[event.owner][i]) {
			event.type = "direction";
			event.x += directX[i];
			event.y += directY[i];
			return;
		}
	}
	
	//bomb event
	for (int i = 0; i < 2; i++) {
		if (event.value == bombKey[event.owner]) {
			event.type = "bomb";
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
	if (event.type == "direction") {
		if (!inMap(event.x, event.y)) {
			return false;
		}

		if (!canGo(event.x, event.y)) {
			return false;
		}

		return true;
	}

	if (event.type == "bomb") {
		if (existedBomb(event.x, event.y)) {
			return false;
		}

		if (countBomb(event.owner) == player[event.owner].numberOfBombs) {
			return false;
		}
			
		return true;
	}

	return false;
}

void updateUserCoordinate(const int& id, const int& newX, const int& newY) {
	player[id].x = newX;
	player[id].y = newY;
}

void addABomb(const tEvent& event) {
	bomb.push_back({event.x, event.y, player[event.owner].bombSize, time(NULL), event.owner});	
}

void removeBlocksInBombLine(const vector<tBomb>& bomb, const vector<bool>& markExplodedBomb) {
	vector<vector<bool>> mark(mapData.row, vector<bool>(mapData.col, false));

	for (int i = 0; i < bomb.size(); i++) {
		if (markExplodedBomb[i] == true) {
			int top, bottom, left, right;
			int x = bomb[i].x;
			int y = bomb[i].y;

			getPosition4Direction(bomb[i], top, bottom, left, right);
		
			mark[x][y] = true;
			mark[top][y] = true;
			mark[bottom][y] = true;
			mark[x][left] = true;
			mark[x][right] = true;
		}
	}	

	for (int i = 0; i < mapData.row; i++) {
		for (int j = 0; j < mapData.col; j++) {
			if (mark[i][j] == true && canBreak(i, j)) {
				mapData.matrix[i][j] = entity[0].symbol;
			}
		}
	}
}

void removeItemsInBombLine(const vector<tBomb>& bomb, const vector<bool>& markExplodedBomb) {
	vector<vector<bool>> mark(mapData.row, vector<bool>(mapData.col, false));

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
		int x = item[i].x;
		int y = item[i].y;
		if (canGo(x, y) && mark[item[i].x][item[i].y] == true) {
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
	for (int i = 0; i < itemList.size(); i++) {
		if (itemList[i].symbol == event.value) {
			item.push_back({event.x, event.y, i});
			return;
		}
	}
}

void processItem(const int& owner, const int& x, const int& y) {
	for (int i = 0; i < item.size(); i++) {
		if (item[i].x == x && item[i].y == y) {

			for (int k = 0; k < itemList[item[i].type].attribute.size(); k++) {
				string attr = itemList[item[i].type].attribute[k];

				if (attr == "bomb") {
					if (player[owner].numberOfBombs < mapData.maxBomb) {
						player[owner].numberOfBombs++;
					}
				} else {
					if (attr == "bombSize") {
						if (player[owner].bombSize < mapData.maxBombSize) {
							player[owner].bombSize++;		
						}
					}

				}
			}
			
			item.erase(item.begin()+i);
			break;
		}
	}	
}

void updateGameInfo(const tEvent& event) {
	if (event.type == "direction") {
		updateUserCoordinate(event.owner, event.x, event.y);		
		processItem(event.owner, event.x, event.y);
	}

	if (event.type == "bomb") {
		addABomb(event);
	}

	//item event
	
	if (event.type == "item") {
		setItem(event);
	}
}

void updateBombStatus() {
	removeExplodedBomb(bomb);
}

bool inBombLine(const int& x, const int& y) {
	vector<bool> markProcessingBomb(bomb.size(), false);

	for (int i = 0; i < bomb.size(); i++) {
		if (bombInProcess(bomb[i])) {
			markConnectedBombs(i, markProcessingBomb);
		}
	}

	for (int i = 0; i < bomb.size(); i++) {
		if (markProcessingBomb[i]) {
			int top, bottom, left, right;
			getPosition4Direction(bomb[i], top, bottom, left, right);
			if (y == bomb[i].y && pointInLine(x, top, bottom)) return true;
			if (x == bomb[i].x && pointInLine(y, left, right)) return true;
		}
	}

	return false;
}


int stopCondition() {

	//timeout
	
	if (mapData.gameDuration < (int) difftime(time(NULL), mapData.startTime)) {
		return 2;
	}

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
	return (item.size() < mapData.maxItem && (int) difftime(time(NULL), mapData.startTime) % mapData.itemDelay == 0); 
}
void endGame(const int& stopCode) {
	render();
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

	for (int i = 0; i < mapData.row; i++) {
		for (int j = 0; j < mapData.col; j++) {
			if (canGo(i, j) || canBreak(i, j)) {
				emptyCell.push_back({i, j});
			}
		}	
	}

	for (int i = emptyCell.size()-1; i >= 0; i--) {
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

	itemType = rand() % itemList.size(); 

	itemPosition = emptyCell[rand() % emptyCell.size()];	

	event = {"item", itemList[itemType].symbol, itemPosition.first, itemPosition.second, 0};
}

void updateGameStatus() {
	while (true) {
		render();
		updateBombStatus();

		mapData.stopCode = stopCondition();
		if (mapData.stopCode != -1) {
			endGame(mapData.stopCode);
			return;
		}

		if (addAnEvent()) {
			tEvent event;
			generateAnItem(event);
			updateGameInfo(event);
		}
	}
}

void gameLoop() {
	sf::Mutex mutex;

	cout << "Game begin." << endl;	

	sf::Thread updateGameStatusThread(&updateGameStatus);
	updateGameStatusThread.launch();

	while (true) {
		tEvent inputEvent;

		do {
			inputEvent = getUserInput();

			if (mapData.stopCode != -1) {
				return;
			}

			updateEventInfo(inputEvent);

		} while (!validInput(inputEvent));
		
		mutex.lock();
		updateGameInfo(inputEvent);
		mutex.unlock();
	}
}

int main() {
	start();
	initiate();

	
	for (int i = 0; i < entity.size(); i++) {
		cout << entity[i].symbol << " " << entity[i].canGoThrough << " " << entity[i].canBreak << " " << entity[i].imgFile << endl;
	}

	for (int i = 0; i < itemList.size(); i++) {
		tItemList it = itemList[i];
		cout << it.symbol << " " << it.attribute.size() << " ";
		for (int j = 0; j < it.attribute.size(); j++) {
			cout << it.attribute[j] << " ";
		}
		cout << endl;
	}
	

	app.create(sf::VideoMode(mapData.row * cellHeight, mapData.col * cellWidth), "BoomOffline2Players - hieutm211", sf::Style::Close);

	while (app.isOpen()) {
		gameLoop();	
		app.close();
	}

	return 0;
}
