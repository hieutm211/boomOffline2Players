
#include <SFML/Graphics.hpp>
#include <cstdio>
#include <iostream>
#include <vector>
#include <fstream>
using namespace std;

const int cellHeight = 60;
const int cellWidth = 60;
int wHeight;
int wWidth;

const string directKey[2][4] = {
	{"W", "S", "A", "D"}, 
	{"Up", "Down", "Left", "Right"}
};

const string bombKey[2] = {"LShift", "RShift"};

const int directX[4] = {-0, +0, -1, +1};
const int directY[4] = {-1, +1, -0, +0};

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
	string type;
	bool canGoThrough;
	bool canBreak;
	string imgFile;
	sf::Texture texture;
	sf::Sprite sprite;
};

struct tPlayer {
	int x;
	int y;
	string name;
	int character;
	int numberOfBombs;
	int bombSize;
	int speed;
	vector<int> direction;
};

struct tBomb {
	int x;
	int y;
	int size;
	time_t timeStart;
	int owner;
	bool goThrough;
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

bool inMap(const int& x, const int& y) {
	return 0 <= x && x < mapData.row && 0 <= y && y < mapData.col;
}

bool inWindow(const int& x, const int& y) {
	return cellWidth/2 <= x && x < wWidth-cellWidth/2+1 && cellHeight/2 <= y && y < wHeight-cellHeight/2+1;
}

bool pointInLine(int x, int L, int R) {
	return L <= x && x <= R;
}

void convertMapXY(int& x, int& y) {
	x = max(0, x);
	y = max(0, y);
	x = min(x, wWidth-1);
	y = min(y, wHeight-1);

	int newX = y / cellHeight;
	int newY = x / cellWidth;

	x = newX;
	y = newY;
}

void convertCenterPoint(int& x, int& y) {
	x = x + cellWidth/2;
	y = y + cellHeight/2;
}

bool canGo(const int& x, const int& y) {
	if (!inMap(x, y)) {
		return false;
	}

	for (int i = 0; i < entity.size(); i++) {
		if (entity[i].symbol == mapData.matrix[x][y] && entity[i].canGoThrough == false) {
			return false;
		}
	}

	for (int i = 0; i < bomb.size(); i++) {
		if (x == bomb[i].x && y == bomb[i].y && bomb[i].goThrough == false) {
			return false;
		}
	}

	return true;
}

bool validWindowPoint(const int& x, const int& y) {
	if (x < 0 || y < 0 || wWidth <= x || wHeight <= y) return false;
	int u = x;
	int v = y;
	convertMapXY(u, v);
	return canGo(u, v);
}

bool canBreak(const int x, const int y) {
	for (int i = 0; i < entity.size(); i++) {
		if (entity[i].symbol == mapData.matrix[x][y]) {
			return entity[i].canBreak;
		}
	}
}

bool checkKeystroke(string key, int id) {
	if (key == bombKey[id]) return true;
	for (int i = 0; i < 4; i++) {
		if (key == directKey[id][i]) return true;
	}	
	return false;
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

bool readMap(const string& mapFileName) {
	print("Read mapData: begin");

	ifstream mapFile;
	mapFile.open(mapFileName);
	if (!mapFile.is_open()) {
		cout << "cannot open file " << mapFileName << endl;
		return false;
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


	string symbol, imgFile;
	bool canGoThrough, canBreak;
	
	//add ground - first entity
	mapFile >> symbol >> canGoThrough >> canBreak >> imgFile;
	entity.push_back({symbol, "ground", canGoThrough, canBreak, imgFile});
	
	for (int i = 1; i < numberOfEntities; i++) {
		mapFile >> symbol >> canGoThrough >> canBreak >> imgFile;
		entity.push_back({symbol, "entity", canGoThrough, canBreak, imgFile});
	}
	//End Read.................................
	
	mapFile.close();
	
	mapData.stopCode = -1;

	return true;
}

bool readCharacter(const string& cFileName) {
	print("reading character: begin");

	ifstream cFile;
	cFile.open(cFileName, ifstream::in);
	if (!cFile.is_open()) {
		cout << "Cannot open cFile " << cFileName << endl;
		return false;
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
		entity.push_back({character[i].symbol, "player", 0, 1, imgFile});
		entity.push_back({bombSymbol, "bomb", 0, 1, bombImgFile});
		entity.push_back({bombLineSymbol, "bombLine", 1, 0, bombLineImgFile});
	}

	cFile.close();

	return true;
} 

bool readItem(string itemFileName) {
	print("read items: begin");

	ifstream iFile;
	iFile.open(itemFileName, ifstream::in);
	if (!iFile.is_open()) {
		cout << "Cannot open iFile " << itemFileName << endl;
		return false;
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
		entity.push_back({itemList[i].symbol, "item", 1, 1, imgFile});
	}

	iFile.close();

	return true;
}

void getInitPosition(string symbol, tPlayer& p) {
	for (int i = 0; i < mapData.row; i++) {
		for (int j = 0; j < mapData.col; j++) {
			if (mapData.matrix[i][j] == symbol) {
				p.x = j*cellWidth + cellWidth/2;
				p.y = i*cellHeight + cellWidth/2;
				mapData.matrix[i][j] = '.';
				return;
			}
		}
	}	
}

bool start() {
	print("starting...");

	cout << "Input mapFile = ";
	cin >> mapFileName;
	mapFileName += ".txt";

	cout << "Input characterFile = ";
	cin >> cFileName;
	cFileName += ".txt";

	cout << "Input itemFile = ";
	cin >> itemFileName;
	itemFileName += ".txt";

	if (!readMap(mapFileName) || !readCharacter(cFileName) || !readItem(itemFileName)) {
		return false;
	}

	wHeight = mapData.row * cellHeight;
	wWidth = mapData.col * cellWidth;

	for (int i = 0; i < entity.size(); i++) {
		if (!entity[i].texture.loadFromFile("images/" + entity[i].imgFile, sf::IntRect(0, 0, cellWidth, cellHeight))) {
			return false;
		}
		entity[i].sprite.setTexture(entity[i].texture);
	}

	return true;
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

	if (top >= 0 && !canBreak(top, bomb.y)) top++;
	if (left >= 0 && !canBreak(bomb.x, left)) left++;
	if (bottom < mapData.row && !canBreak(bottom, bomb.y)) bottom--;
	if (right < mapData.col && !canBreak(bomb.x, right)) right--;

	top = max(0, top);
	left = max(0, left);
	bottom = min(mapData.row-1, bottom);
	right = min(mapData.col-1, right);
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
		convertMapXY(x, y);
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

void renderConsole() {
	double difft = difftime(time(NULL), mapData.startTime);
	if (difft - (int) difft < 0.01) system("clear");

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

	for (int i = 0; i < player.size(); i++) {
		cout << "Player " << i << ": ";
		for (int x : player[i].direction) {
			cout << x << " ";	
		}
		cout << endl;
	}
	cout << endl;
	for (int i = 0; i < player.size(); i++) {
		cout << "Player " << i << ": \n";
		cout << "    " << "Coordinates: " << player[i].x << " " << player[i].y << endl;
		cout << "    " << "Number of bombs: " << player[i].numberOfBombs << endl;
		cout << "    " << "BombSize: " << player[i].bombSize << endl;
		cout << "    " << "Speed: " << player[i].speed << endl;
	}

	cout << endl;

	cout << "Bombs: ";
	for (tBomb& b: bomb) {
		cout << "(" << b.x << ", " << b.y << ", owner = " << b.owner << "), ";
	}
	cout << endl;

	//cout << "Items: ";
	//for (tItem& i: item) {
	//	cout << "(" << i.x << ", " << i.y << ", type = " << i.type << "), ";
	//}
}

void renderUI() {
	vector<vector<string>> mapBind;
	bind(mapData, player, bomb, item, mapBind);

	//draw
	app.clear(sf::Color::White);

	//draw ground	
	for (int i = 0; i < mapData.row; i++) {
		for (int j = 0; j < mapData.col; j++) {
			entity[0].sprite.setPosition(sf::Vector2f(j*cellWidth, i*cellHeight));
			app.draw(entity[0].sprite);
		}
	}

	for (int i = 0; i < player.size(); i++) {
		for (int k = 0; k < entity.size(); k++) {
			if (entity[k].type == "player" && character[player[i].character].symbol == entity[k].symbol) {
				entity[k].sprite.setPosition(sf::Vector2f(player[i].x - cellWidth/2, player[i].y - cellHeight/2));
				app.draw(entity[k].sprite);
				break;
			} 
		}
	}

	for (int k = 0; k < entity.size(); k++) {
		if (entity[k].type != "ground" && entity[k].type != "player") {

			for (int i = 0; i < mapData.row; i++) {
				for (int j = 0; j < mapData.col; j++) {
					if (mapBind[i][j] == entity[k].symbol) {
						entity[k].sprite.setPosition(sf::Vector2f(j*cellWidth, i*cellHeight));
						app.draw(entity[k].sprite);
					}
				}
			}
		}
	}

	app.display();
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

string getDirectionReleased(const sf::Event& event) {
	if (event.type != sf::Event::KeyReleased) return "null";

	switch (event.key.code) {
		case sf::Keyboard::W : return "W";
		case sf::Keyboard::S : return "S";
		case sf::Keyboard::A : return "A";
		case sf::Keyboard::D : return "D";
	
		case sf::Keyboard::Up : return "Up";
		case sf::Keyboard::Down : return "Down";
		case sf::Keyboard::Left : return "Left";
		case sf::Keyboard::Right : return "Right";

		default:
			return "null";
	}
}

void updateEventInfo(tEvent& event) {
	//item event
	if (event.owner == 2) {
		return;
	}

	//direct event
	for (int i = 0; i < 4; i++) {
		if (event.value == directKey[event.owner][i]) {
			if (event.type == "?") event.type = "direction";
			event.x += directX[i] * player[event.owner].speed;
			event.y += directY[i] * player[event.owner].speed;
			return;
		}
	}
	
	//bomb event
	for (int i = 0; i < 2; i++) {
		if (event.value == bombKey[event.owner]) {
			event.type = "bomb";
			convertMapXY(event.x, event.y);
			return;
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

void convertCorner(const int& cornerID, int& x, int& y) {
	int newX, newY;

	switch (cornerID) {
		case 0 : 
			newX = x + cellWidth/2 - 1;
			newY = y - cellHeight/2;
			break;
		case 1 : 
			newX = x - cellWidth/2;
			newY = y - cellHeight/2;
			break;
		case 2 :
			newX = x - cellWidth/2;
			newY = y + cellHeight/2 - 1;
			break;
		case 3 : 
			newX = x + cellWidth/2 - 1;
			newY = y + cellHeight/2 - 1;
			break;
		default :
			return;
	}
	x = newX;
	y = newY;
}

bool validPlayerPosition(const int& x, const int& y) {
		if (!inWindow(x, y)) return false;

		int xMap, yMap;

		//check 4 corner
		for (int i = 0; i < 4; i++) {
			xMap = x;
			yMap = y;
				
			convertCorner(i, xMap, yMap);
			convertMapXY(xMap, yMap);

			if (!canGo(xMap, yMap)) {
				return false;
			}

		}

		return true;
}

bool validInput(const tEvent& event) {
	if (event.type == "direction") {
		if (!inWindow(event.x, event.y)) {
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

void addBomb(const tEvent& event) {
	bomb.push_back({event.x, event.y, player[event.owner].bombSize, time(NULL), event.owner, true});	
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

bool isFourCornerOutside(const int& xWindow, const int& yWindow, const int& xMap, const int& yMap) {
	int xCorner, yCorner;

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 2; j++) {
			if (i == 0) xCorner = xWindow - cellWidth/2;
			else xCorner = xWindow + cellWidth/2 - 1;

			if (j == 0) yCorner = yWindow - cellHeight/2;
			else yCorner = yWindow + cellHeight/2 -1;

			convertMapXY(xCorner, yCorner);
			if (xCorner == xMap && yCorner == yMap) return false;
		}
	}

	return true;
}

void updateGoThrough(vector<tBomb>& bomb) {
	bool existPlayer;

	for (int i = 0; i < bomb.size(); i++) {
		existPlayer = false;
		for (int playerID = 0; playerID < player.size(); playerID++) {
			if (isFourCornerOutside(player[playerID].x, player[playerID].y, bomb[i].x, bomb[i].y) == false) {
				existPlayer = true;
			}
		}
		if (existPlayer == false) {
			bomb[i].goThrough = false;
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

void processItem(const int& owner, int x, int y) {
	convertMapXY(x, y);

	for (int i = 0; i < item.size(); i++) {
		if (item[i].x == x && item[i].y == y) {

			for (int k = 0; k < itemList[item[i].type].attribute.size(); k++) {
				string attr = itemList[item[i].type].attribute[k];

				if (attr == "bomb") {
					if (player[owner].numberOfBombs < mapData.maxBomb) {
						player[owner].numberOfBombs++;
					}
				} else if (attr == "bombSize") {
					if (player[owner].bombSize < mapData.maxBombSize) {
						player[owner].bombSize++;		
					}
				} else if (attr == "speed") {
					if (player[owner].speed < mapData.maxSpeed) {
						player[owner].speed++;
					}
				}
			}
			
			item.erase(item.begin()+i);
			break;
		}
	}	
}

void removeDirectElement(vector<int>& directVec, const int d) {
	for (int i = directVec.size()-1; i >= 0; i--) {
		if (directVec[i] == d) {
			directVec.erase(directVec.begin() + i);
		}
	}
}

void updateGameInfo(const tEvent& event) {
	if (event.type == "direction") {
		if (event.value == "W") player[event.owner].direction.push_back(0);
		if (event.value == "S") player[event.owner].direction.push_back(1);
		if (event.value == "A") player[event.owner].direction.push_back(2);
		if (event.value == "D") player[event.owner].direction.push_back(3);

		if (event.value == "Up") player[event.owner].direction.push_back(0);
		if (event.value == "Down") player[event.owner].direction.push_back(1);
		if (event.value == "Left") player[event.owner].direction.push_back(2);
		if (event.value == "Right") player[event.owner].direction.push_back(3);
	}

	if (event.type == "direction released") {
		if (event.value == "W") removeDirectElement(player[event.owner].direction, 0);
		if (event.value == "S") removeDirectElement(player[event.owner].direction, 1);
		if (event.value == "A") removeDirectElement(player[event.owner].direction, 2);
		if (event.value == "D") removeDirectElement(player[event.owner].direction, 3);

		if (event.value == "Up") removeDirectElement(player[event.owner].direction, 0);
		if (event.value == "Down") removeDirectElement(player[event.owner].direction, 1);
		if (event.value == "Left") removeDirectElement(player[event.owner].direction, 2);
		if (event.value == "Right") removeDirectElement(player[event.owner].direction, 3);
	}

	if (event.type == "bomb") {
		addBomb(event);
	}

	//item event
	
	if (event.type == "item") {
		setItem(event);
	}
}

void updateBombStatus() {
	removeExplodedBomb(bomb);
	updateGoThrough(bomb);
}

bool inBombLine(const tPlayer& player) {
	vector<bool> markProcessingBomb(bomb.size(), false);

	for (int i = 0; i < bomb.size(); i++) {
		if (bombInProcess(bomb[i])) {
			markConnectedBombs(i, markProcessingBomb);
		}
	}

	int x = player.x;
	int y = player.y;
	convertMapXY(x, y);

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

	bool player0dead = inBombLine(player[0]);
	bool player1dead = inBombLine(player[1]);

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

bool addEvent() {
	return (item.size() < mapData.maxItem && (int) difftime(time(NULL), mapData.startTime) % mapData.itemDelay == 0); 
}

void endGame(const int& stopCode) {
	renderConsole();
	renderUI();
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

void processDirection(const int& playerID, int k) {
	for (int loop = 0; loop < player[playerID].speed; loop++) {
		int newX = player[playerID].x + directX[k];
		int newY = player[playerID].y + directY[k];

		newX = max(cellWidth/2, newX);
		newY = max(cellHeight/2, newY);

		newX = min(newX, wWidth - cellWidth/2);
		newY = min(newY, wHeight - cellHeight/2);

		if (validPlayerPosition(newX, newY)) {
			updateUserCoordinate(playerID, newX, newY); 	
			processItem(playerID, newX, newY);
		} else {

			int xCorner, yCorner;

			if (validWindowPoint(newX, newY)) {
				cout << "before: " << newX << " " << newY << endl;
				int countBlockedCorner = 0;
				for (int corner = 0; corner < 4; corner++) {
					xCorner = newX;
					yCorner = newY;
					convertCorner(corner, xCorner, yCorner);

					if (!validWindowPoint(xCorner, yCorner)) {
						countBlockedCorner++;
					}	
					cout << xCorner << " " << yCorner << " " << countBlockedCorner << endl;
				}
				if (countBlockedCorner == 1) {
					for (int corner = 0; corner < 4; corner++) {
						xCorner = newX;
						yCorner = newY;
						convertCorner(corner, xCorner, yCorner);

						if (!validWindowPoint(xCorner, yCorner)) {
							switch(corner) {
								case 0: 
									
									if (k == 0) { //Up
										newX = newX - 1; 
										newY = newY - directY[k];
									}
									if (k == 3) { //Right
										newX = newX - directX[k];
										newY = newY + 1;
									}
									break;

								case 1: 
									if (k == 0) { //Up
										newX = newX + 1; 
										newY = newY - directY[k];
									}
									if (k == 2) { //Left
										newX = newX - directX[k];
										newY = newY + 1;
									}
									break;

								case 2:
									if (k == 1) { //Down
										newX = newX + 1; 
										newY = newY - directY[k];
									}
									if (k == 2) { //Left
										newX = newX - directX[k];
										newY = newY - 1;
									}

									break;

								case 3:
									if (k == 1) { //Down
										newX = newX - 1; 
										newY = newY - directY[k];
									}
									if (k == 3) { //Right
										newX = newX - directX[k];
										newY = newY - 1;
									}
									break;
							}
						}
					}
					cout << "after: " << newX << " " << newY << endl;
					updateUserCoordinate(playerID, newX, newY); 	
					processItem(playerID, newX, newY);
				}
			}
		}
	}
}

void updateGameStatus() {
		renderConsole();
		renderUI();
		updateBombStatus();

		mapData.stopCode = stopCondition();
		if (mapData.stopCode != -1) {
			endGame(mapData.stopCode);
			return;
		}

		if (addEvent()) {
			tEvent event;
			generateAnItem(event);
			updateGameInfo(event);
		}

		for (int i = 0; i < player.size(); i++) {
			if (player[i].direction.size() > 0) {
				processDirection(i, player[i].direction.back());
				int k = player[i].direction.back();
			}
		}
}

void addEventInfo(tEvent& event) {
	for (int i = 0; i < 2; i++) {
		if (checkKeystroke(event.value, i)) {
			event.x = player[i].x;
			event.y = player[i].y;
			event.owner = i;
		}
	}
}

int main() {
	
	if (start() == true) {
		print("read data successfully.");
		initiate();

		print("\nInitiate successfully. Starting game...\n");
		system("sleep 1");
	} else {
		print("fail to load data.");
		return 0;
	};

	app.create(sf::VideoMode(mapData.row * cellHeight, mapData.col * cellWidth), "BoomOffline2Players - hieutm211", sf::Style::Close);
	app.setFramerateLimit(120);
	app.setKeyRepeatEnabled(false);
	
	while (app.isOpen()) {
		sf::Event sfEvent;
		tEvent inputEvent;
			if (mapData.stopCode != -1) {
				endGame(mapData.stopCode);
				return false;
			}

		while (app.pollEvent(sfEvent)) {
			if (sfEvent.type == sf::Event::Closed) {
				app.close();
				return 0;
			}
			
			inputEvent.value = getKeyPress(sfEvent);

			if (inputEvent.value != "null") {
				inputEvent.type = "?";
				addEventInfo(inputEvent);
				updateEventInfo(inputEvent);

				if (validInput(inputEvent)) {
					updateGameInfo(inputEvent);
				}
			}

			inputEvent.value = getDirectionReleased(sfEvent);

			if (inputEvent.value != "null") {
				inputEvent.type = "direction released";
				addEventInfo(inputEvent);
				updateEventInfo(inputEvent);
				updateGameInfo(inputEvent);
			}
		}

		updateGameStatus();
	}
	
    return 0;
}
