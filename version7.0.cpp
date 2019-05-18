
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cstdio>
#include <iostream>
#include <vector>
#include <fstream>

using namespace std;

const int framerateLimit = 120;
const int cellHeight = 60;
const int cellWidth = 60;
int wHeight;
int wWidth;
int gameTick = 0;

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

class tPoint {
public:
	int x, y;

	tPoint() {
		x = -1;
		y = -1;
	}
	
	tPoint(int _x, int _y) {
		x = _x;
		y = _y;
	}

	void update(int _x, int _y) {
		x = _x;
		y = _y;
	}

	void update(const tPoint& other) {
		x = other.x;
		y = other.y;
	}

	bool inMap();
	bool inWindow();
	bool canGo();
	bool validWindowPoint();
	bool canBreak();
	bool validPlayerPosition();
	bool existedBomb();

	tPoint corner(int cornerID);
	void convertCorner(int cornerID);

	tPoint mapXY();
	void convertMapXY();

	tPoint windowXY();

	tPoint centerPoint();
	void convertCenterPoint();

	void print(string note);
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
	tPoint position;
	string name;
	int id;
	int character;
	int numberOfBombs;
	int bombSize;
	int speed;
	vector<string> direction;
	
	int countBomb();
	void processItem();
	void addDirection(string d);
	void removeDirection(string d);
	string getDirection();
	int getDirectionCode();
	void processDirection(int d);
	bool inBombLine();
};

struct tBomb {
	tPoint position;	
	int size;
	time_t timeStart;
	int owner;
	bool goThrough;

	double currentDuration() const;
	bool inProcess() const;
	bool isExploded() const;
};

struct tItem {
	tPoint position;
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
	tPoint position;
	int owner;

	void updateInfo();
	void addInfo();
	bool isValid();
	void generateItem();
};

struct tItemList {
	string symbol;
	vector<string> attribute;
};

sf::RenderWindow app;
sf::Music music;
sf::Music grenadeExplodeSound;
sf::Texture stopCodeTexture[3];
sf::Sprite stopCodeSprite[3];

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

bool tPoint::inMap() {
	return 0 <= x && x < mapData.row && 0 <= y && y < mapData.col;
}

bool tPoint::inWindow() {
	return cellWidth/2 <= x && x < wWidth-cellWidth/2+1 && cellHeight/2 <= y && y < wHeight-cellHeight/2+1;
}

tPoint tPoint::corner(int cornerID) {
	tPoint newP = {-1, -1};
	
	switch (cornerID) {
		case 0 : 
			newP.x = x + cellWidth/2 - 1;
			newP.y = y - cellHeight/2;
			break;
		case 1 : 
			newP.x = x - cellWidth/2;
			newP.y = y - cellHeight/2;
			break;
		case 2 :
			newP.x = x - cellWidth/2;
			newP.y = y + cellHeight/2 - 1;
			break;
		case 3 : 
			newP.x = x + cellWidth/2 - 1;
			newP.y = y + cellHeight/2 - 1;
			break;
	}
	return newP;
}

void tPoint::convertCorner(int cornerID) {
	tPoint newP = this->corner(cornerID);
	this->x = newP.x;
	this->y = newP.y;
}

void tPoint::print(string note) {
	cout << note << " (" << this->x << ", " << this->y << ");" << endl; 
}

bool pointInLine(int x, int L, int R) {
	return L <= x && x <= R;
}

tPoint tPoint::mapXY() {
	x = max(0, x);
	y = max(0, y);
	x = min(x, wWidth-1);
	y = min(y, wHeight-1);

	tPoint newP;
	newP.x = y / cellHeight;
	newP.y = x / cellWidth;

	return tPoint(newP);
}

void tPoint::convertMapXY() {
	tPoint newP = this->mapXY();
	this->x = newP.x;
	this->y = newP.y;
}

tPoint tPoint::windowXY() {
	tPoint newP;
	newP.x = y * cellHeight;
	newP.y = x * cellWidth;
	return newP;
}

tPoint tPoint::centerPoint() {
	x = x + cellWidth/2;
	y = y + cellHeight/2;
	return tPoint(x, y);
}

void tPoint::convertCenterPoint() {
	tPoint newP = this->centerPoint();
	this->x = newP.x;
	this->y = newP.y;
}

bool tPoint::canGo() {
	if (!this->inMap()) {
		return false;
	}

	for (int i = 0; i < entity.size(); i++) {
		if (entity[i].symbol == mapData.matrix[x][y] && entity[i].canGoThrough == false) {
			return false;
		}
	}

	for (int i = 0; i < bomb.size(); i++) {
		if (x == bomb[i].position.x && y == bomb[i].position.y && bomb[i].goThrough == false) {
			return false;
		}
	}

	return true;
}

bool tPoint::validWindowPoint() {
	if (x < 0 || y < 0 || wWidth <= x || wHeight <= y) return false;
	return this->mapXY().canGo();
}

bool tPoint::canBreak() {
	for (int i = 0; i < entity.size(); i++) {
		if (entity[i].symbol == mapData.matrix[x][y]) {
			return entity[i].canBreak;
		}
	}
}

void print(string s) {
	cout << s << endl;
}

bool checkKeystroke(string key, int id) {
	if (key == bombKey[id]) return true;
	for (int i = 0; i < 4; i++) {
		if (key == directKey[id][i]) return true;
	}	
	return false;
}

int abs(const int& a, const int& b) {
	if (a >= b) return a - b;
	return b - a;	
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
				p.position.x = j*cellWidth + cellWidth/2;
				p.position.y = i*cellHeight + cellWidth/2;
				mapData.matrix[i][j] = '.';
				return;
			}
		}
	}	
}

bool start() {
	print("starting...");

	cout << "Input mapFile = ";
	//cin >> mapFileName;
	//mapFileName += ".txt";
	mapFileName = "mapData.txt";

	cout << "Input characterFile = ";
	//cin >> cFileName;
	//cFileName += ".txt";
	cFileName = "characterData.txt";

	cout << "Input itemFile = ";
	//cin >> itemFileName;
	//itemFileName += ".txt";
	itemFileName = "itemData.txt";

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

	if (!music.openFromFile("sounds/background.wav")) {
		cout << "cannot load background wav" << endl;
		return;
	}
	music.setLoop(true);

	if (!grenadeExplodeSound.openFromFile("sounds/grenadeExplode.wav")) {
		cout << "cannot load grenadeExplode wav" << endl;
		return;
	}

	stopCodeTexture[0].loadFromFile("images/draw.png", sf::IntRect(0, 0, 6*cellWidth, 3*cellHeight));
	stopCodeSprite[0].setTexture(stopCodeTexture[0]);

	stopCodeTexture[1].loadFromFile("images/player1win.png", sf::IntRect(0, 0, 6*cellWidth, 3*cellHeight));
	stopCodeSprite[1].setTexture(stopCodeTexture[1]);

	stopCodeTexture[2].loadFromFile("images/player2win.png", sf::IntRect(0, 0, 6*cellWidth, 3*cellHeight));
	stopCodeSprite[2].setTexture(stopCodeTexture[2]);

	for (int i = 0; i < 2; i++) {
		player[i].id = i;
		player[i].character = i;
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

double tBomb::currentDuration() const {
	time_t timeNow = time(NULL);

	return difftime(timeNow, this->timeStart);
}

bool tBomb::inProcess() const {
	double currentDuration = this->currentDuration(); 	
	return mapData.bombDuration < currentDuration && currentDuration <= mapData.bombDuration + mapData.bombProcessTime;
}

bool tBomb::isExploded() const {
	double currentDuration = this->currentDuration();
	return mapData.bombDuration + mapData.bombProcessTime < currentDuration;
}

void getPosition4Direction(const tBomb& bomb, int& top, int& bottom, int& left, int& right) {
	top = bomb.position.x-1;
	bottom = bomb.position.x+1;
	left = bomb.position.y-1;
	right = bomb.position.y+1;

	for (int i = 0; i < bomb.size-1; i++) {
		if (top-1 >= 0 && tPoint(top, bomb.position.y).canGo()) top--;
		if (bottom+1 < mapData.row && tPoint(bottom, bomb.position.y).canGo()) bottom++;
		if (left-1 >= 0 && tPoint(bomb.position.x, left).canGo()) left--;
		if (right+1 < mapData.col && tPoint(bomb.position.x, right).canGo()) right++;
	}

	if (top >= 0 && !tPoint(top, bomb.position.y).canBreak()) top++;
	if (left >= 0 && !tPoint(bomb.position.x, left).canBreak()) left++;
	if (bottom < mapData.row && !tPoint(bottom, bomb.position.y).canBreak()) bottom--;
	if (right < mapData.col && !tPoint(bomb.position.x, right).canBreak()) right--;

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
		if (bomb[i].position.x == bomb[k].position.x && pointInLine(bomb[i].position.y, left, right)) markConnectedBombs(i, mark);
		if (bomb[i].position.y == bomb[k].position.y && pointInLine(bomb[i].position.x, top, bottom)) markConnectedBombs(i, mark);
	}
}

void fillBomb(const vector<tBomb>& bom, vector<vector<string>>& mapBind) {
	int x, y;

	for (int i = 0; i < bomb.size(); i++) {
		x = bomb[i].position.x;
		y = bomb[i].position.y;

		mapBind[x][y] = (char)('a' + bomb[i].owner);
	}
}

void fillBombLine(const vector<tBomb>& bomb, vector<vector<string>>& mapBind) {
	vector<bool> markProcessingBomb(bomb.size(), false);

	for (int i = 0; i < bomb.size(); i++) {
		if (bomb[i].inProcess()) {
			markConnectedBombs(i, markProcessingBomb);
		}
	}

	for (int k = 0; k < bomb.size(); k++) {
		if (markProcessingBomb[k] == true) {
			char bombChar = (char) ('0' + bomb[k].owner);
			int top, bottom, left, right;

			int x = bomb[k].position.x;
			int y = bomb[k].position.y;

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
		int x = item[i].position.x;
		int y = item[i].position.y;
		
		mapBind[x][y] = itemList[item[i].type].symbol;
	}
}

void fillPlayer(const vector<tPlayer>& player, vector<vector<string>>& mapBind) {
	tPoint pTemp;
	for (int i = 0; i < player.size(); i++) {
		pTemp.update(player[i].position.x, player[i].position.y);
		pTemp.convertMapXY();
		mapBind[pTemp.x][pTemp.y] = character[player[i].character].symbol;
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
	if (gameTick % 1 == 0) system("clear");

	//print time left
	
	cout << "Game Tick: " << gameTick << endl;
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
		for (string x : player[i].direction) {
			cout << x << " ";	
		}
		cout << endl;
	}
	cout << endl;

	for (int i = 0; i < player.size(); i++) {
		cout << "Player " << i << ": \n";
		cout << "    " << "Coordinates: " << player[i].position.x << " " << player[i].position.y << endl;
		cout << "    " << "Number of bombs: " << player[i].numberOfBombs << endl;
		cout << "    " << "BombSize: " << player[i].bombSize << endl;
		cout << "    " << "Speed: " << player[i].speed << endl;
		cout << endl;
	}

	cout << endl;


	cout << "Bombs: ";
	for (tBomb& b: bomb) {
		cout << "(" << b.position.x << ", " << b.position.y << ", owner = " << b.owner << "), ";
	}
	cout << endl;

	cout << "Items: ";
	for (tItem& i: item) {
		cout << "(" << i.position.x << ", " << i.position.y << ", type = " << i.type << "), ";
	}
	cout << endl;
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
				entity[k].sprite.setPosition(sf::Vector2f(player[i].position.x - cellWidth/2, player[i].position.y - cellHeight/2));
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

	if (mapData.stopCode != -1) {
		if (mapData.stopCode == player.size()) {
			stopCodeSprite[0].setPosition(sf::Vector2f(mapData.col * cellWidth/2 - 6*cellWidth/2, mapData.row * cellHeight/2 - 3*cellHeight));
			app.draw(stopCodeSprite[0]);
		} else {
			int winner = mapData.stopCode+1;
			stopCodeSprite[winner].setPosition(sf::Vector2f(mapData.col * cellWidth/2 - 6*cellWidth/2, mapData.row * cellHeight/2 - 3*cellHeight));
			app.draw(stopCodeSprite[winner]);
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

void tEvent::updateInfo() {
	//item event
	if (this->owner == 2) {
		return;
	}

	//direct event
	for (int i = 0; i < 4; i++) {
		if (this->value == directKey[this->owner][i]) {
			this->position.x += directX[i] * player[this->owner].speed;
			this->position.y += directY[i] * player[this->owner].speed;
			return;
		}
	}
	
	//bomb event
	for (int i = 0; i < 2; i++) {
		if (this->value == bombKey[this->owner]) {
			this->type = "bomb";
			this->position.convertMapXY();
			return;
		}
	}
}

bool tPoint::existedBomb() {
	for (int i = 0; i < bomb.size(); i++) {
		if (bomb[i].position.x == x && bomb[i].position.y == y) {
			return true;
		}
	}
	return false;
}

int tPlayer::countBomb() {
	int ans = 0;
	for (int i = 0; i < bomb.size(); i++) {
		if (bomb[i].owner == this->id) {
			ans++;
		}
	}
	return ans;
}

bool tPoint::validPlayerPosition() {
		if (!this->inWindow()) return false;

		//check 4 corner
		for (int i = 0; i < 4; i++) {
			if (!tPoint(x, y).corner(i).mapXY().canGo()) {
				return false;
			}

		}

		return true;
}

bool tEvent::isValid() {
	tPoint pTemp(this->position);
	if (this->type == "direction") {
		if (!pTemp.inWindow()) {
			return false;
		}
		
		return true;
	}

	if (this->type == "bomb") {
		if (pTemp.existedBomb()) {
			return false;
		}

		if (player[this->owner].countBomb() == player[this->owner].numberOfBombs) {
			return false;
		}
			
		return true;
	}

	return false;
}

void addBomb(const tEvent& event) {
	bomb.push_back({event.position, player[event.owner].bombSize, time(NULL), event.owner, true});	
}

void removeBlocksInBombLine(const vector<tBomb>& bomb, const vector<bool>& markExplodedBomb) {
	vector<vector<bool>> mark(mapData.row, vector<bool>(mapData.col, false));
	tPoint pTemp;

	for (int i = 0; i < bomb.size(); i++) {
		if (markExplodedBomb[i] == true) {
			int top, bottom, left, right;
			int x = bomb[i].position.x;
			int y = bomb[i].position.y;

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
			pTemp.update(i, j);
			if (mark[i][j] == true && pTemp.canBreak()) {
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
			int x = bomb[i].position.x;
			int y = bomb[i].position.y;

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
		int x = item[i].position.x;
		int y = item[i].position.y;
		if (mark[item[i].position.x][item[i].position.y] == true) {
			item.erase(item.begin() + i);
		}
	}
}

void removeExplodedBomb(vector<tBomb>& bomb) {
	vector<bool> markExplodedBomb(bomb.size(), false);

	for (int i = 0; i < bomb.size(); i++) {
		if (bomb[i].isExploded()) {
			markConnectedBombs(i, markExplodedBomb);
		}
	} 
	
	removeItemsInBombLine(bomb, markExplodedBomb);
	removeBlocksInBombLine(bomb, markExplodedBomb);

	for (int i = bomb.size()-1; i >= 0; i--) {
		if (markExplodedBomb[i]) {
			grenadeExplodeSound.play();
			bomb.erase(bomb.begin() + i);
		}
	}

}

bool isFourCornerOutside(const int& xWindow, const int& yWindow, const int& xMap, const int& yMap) {
	tPoint pTemp;

	for (int i = 0; i < 4; i++) {
		pTemp.update(xWindow, yWindow);
		pTemp.convertCorner(i);
		pTemp.convertMapXY();
		if (pTemp.x == xMap && pTemp.y == yMap) return false;
	
	}

	return true;
}

void updateGoThrough(vector<tBomb>& bomb) {
	bool existPlayer;

	for (int i = 0; i < bomb.size(); i++) {
		existPlayer = false;
		for (int playerID = 0; playerID < player.size(); playerID++) {
			if (isFourCornerOutside(player[playerID].position.x, player[playerID].position.y, bomb[i].position.x, bomb[i].position.y) == false) {
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
			item.push_back({event.position, i});
			return;
		}
	}
}

void tPlayer::processItem() {
	tPoint pTemp(this->position);
	pTemp.convertMapXY();

	for (int i = 0; i < item.size(); i++) {
		if (item[i].position.x == pTemp.x && item[i].position.y == pTemp.y) {

			for (int k = 0; k < itemList[item[i].type].attribute.size(); k++) {
				string attr = itemList[item[i].type].attribute[k];

				if (attr == "bomb") {
					if (this->numberOfBombs < mapData.maxBomb) {
						this->numberOfBombs++;
					}
				} else if (attr == "bombSize") {
					if (this->bombSize < mapData.maxBombSize) {
						this->bombSize++;		
					}
				} else if (attr == "speed") {
					if (this->speed < mapData.maxSpeed) {
						this->speed++;
					}
				}
			}
			
			item.erase(item.begin()+i);
			break;
		}
	}	
}

void tPlayer::addDirection(string d) {
	if (d == "W") d = "Up";
	if (d == "S") d = "Down";
	if (d == "A") d = "Left";
	if (d == "D") d = "Right";

	this->direction.push_back(d);
}

void tPlayer::removeDirection(string d) {
	if (d == "W") d = "Up";
	if (d == "S") d = "Down";
	if (d == "A") d = "Left";
	if (d == "D") d = "Right";

	for (int i = this->direction.size()-1; i >= 0; i--) {
		if (this->direction[i] == d) {
			this->direction.erase(this->direction.begin() + i);
		}
	}
}

string tPlayer::getDirection() {
	if (this->direction.empty()) {
		return "null";
	}
	

	return this->direction.back();	
}

int tPlayer::getDirectionCode() {
	if (this->direction.empty()) {
		return -1;
	}

	string d = this->direction.back();

	if (d == "Up") return 0;
	if (d == "Down") return 1;
	if (d == "Left") return 2;
	if (d == "Right") return 3;
}

void updateGameInfo(const tEvent& event) {
	if (event.type == "direction") {
		player[event.owner].addDirection(event.value);
	}

	if (event.type == "direction released") {
		player[event.owner].removeDirection(event.value);
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

bool tPlayer::inBombLine() {
	vector<bool> markProcessingBomb(bomb.size(), false);

	for (int i = 0; i < bomb.size(); i++) {
		if (bomb[i].inProcess()) {
			markConnectedBombs(i, markProcessingBomb);
		}
	}

	tPoint pTemp(this->position);
	pTemp.convertMapXY();

	for (int i = 0; i < bomb.size(); i++) {
		if (markProcessingBomb[i]) {
			int top, bottom, left, right;
			getPosition4Direction(bomb[i], top, bottom, left, right);
			if (pTemp.y == bomb[i].position.y && pointInLine(pTemp.x, top, bottom)) return true;
			if (pTemp.x == bomb[i].position.x && pointInLine(pTemp.y, left, right)) return true;
		}
	}

	return false;
}


int stopCondition() {

	//timeout
	
	if (mapData.gameDuration < (int) difftime(time(NULL), mapData.startTime)) {
		return player.size();
	}

	vector<bool> isDead(player.size(), false);
	int alive = 0;

	for (int i = 0; i < isDead.size(); i++) {
		isDead[i] = player[i].inBombLine();
		if (isDead[i] == false) {
			alive++;
		}
	}

	if (alive == 0) {
		return player.size();
	}

	if (alive == 1) {
		for (int i = 0; i < player.size(); i++) {
			if (!isDead[i]) {
				return i;
			}
		}
	}

	return -1;	
}

bool addEvent() {
	return (item.size() < mapData.maxItem && gameTick % (mapData.itemDelay * framerateLimit) == 0); 
}

void endGame(const int& stopCode) {
	renderConsole();
	renderUI();

	if (stopCode == -1) {
		cout << "Draw!" << endl;
	} else {
		cout << "Player " << stopCode << " win!" << endl;
	}
	system("sleep 5");
}

void tEvent::generateItem() {
	vector<tPoint> emptyCell;
	tPoint pTemp;

	for (int i = 0; i < mapData.row; i++) {
		for (int j = 0; j < mapData.col; j++) {
			pTemp.update(i, j);
			if (pTemp.canGo() || pTemp.canBreak()) {
				emptyCell.push_back(pTemp);
			}
		}	
	}

	for (int i = emptyCell.size()-1; i >= 0; i--) {
		for (int j = 0; j < item.size(); j++) {
			if (emptyCell[i].x == item[j].position.x && emptyCell[i].y == item[j].position.y) {
				emptyCell.erase(emptyCell.begin()+i);
				break;
			}
		}
	}

	srand(time(NULL));

	int itemType;	

	itemType = rand() % itemList.size(); 

	this->type = "item";
	this->value = itemList[itemType].symbol;
	this->position = emptyCell[rand() % emptyCell.size()];
	this->owner = 0;
}

void adjustPosition(tPoint& newP, tPoint& originP, const int& d, const int& collisionCorner) {
	tPoint collisionBlockP = newP.corner(collisionCorner).mapXY().windowXY().centerPoint().corner((collisionCorner+2)%4);
	
	int absX = abs(originP.corner(collisionCorner).x, collisionBlockP.x);
	int absY = abs(originP.corner(collisionCorner).y, collisionBlockP.y);

	newP = originP;

	switch(collisionCorner) {
		case 0: 

			if (d == 0) { //Up
				if (absX < cellWidth/3) {
					newP.x -= 1; 
				}
			}
			if (d == 3) { //Right
				if (absY < cellHeight/3) {
					newP.y += 1;
				}
			}
			break;

		case 1: 
			if (d == 0) { //Up
				if (absX < cellWidth/3) {
					newP.x += 1; 
				}
			}
			if (d == 2) { //Left
				if (absY < cellHeight/3) {
					newP.y += 1;
				}
			}
			break;

		case 2:
			if (d == 1) { //Down
				if (absX < cellWidth/3) {
					newP.x += 1; 
				}
			}
			if (d == 2) { //Left
				if (absY < cellHeight/3) {
					newP.y -= 1;
				}
			}

			break;

		case 3:
			if (d == 1) { //Down
				if (absX < cellWidth/3) {
					newP.x -= 1; 
				}
			}
			if (d == 3) { //Right
				if (absY < cellHeight/3) {
					newP.y -= 1;
				}
			}
			break;
	}
}

void tPlayer::processDirection(int d) {
	tPoint newP;
	for (int loop = 0; loop < this->speed; loop++) {
		newP.x = this->position.x + directX[d];
		newP.y = this->position.y + directY[d];

		newP.x = max(cellWidth/2, newP.x);
		newP.y = max(cellHeight/2, newP.y);

		newP.x = min(newP.x, wWidth - cellWidth/2);
		newP.y = min(newP.y, wHeight - cellHeight/2);

		if (newP.validPlayerPosition()) {
			this->position.update(newP); 	
			this->processItem();
		} else {

			if (newP.validWindowPoint()) {
				int countBlockedCorner = 0;
				for (int cornerID = 0; cornerID < 4; cornerID++) {
					if (!newP.corner(cornerID).validWindowPoint()) {
						countBlockedCorner++;
					}	
				}
				if (countBlockedCorner == 1) {
					for (int cornerID = 0; cornerID < 4; cornerID++) {
						if (!newP.corner(cornerID).validWindowPoint()) {
							adjustPosition(newP, this->position, d, cornerID);
						}
					}

					if (newP.validPlayerPosition()) {
						this->position.update(newP);
						this->processItem();
					}
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
			return;
		}

		if (addEvent()) {
			tEvent event;
			event.generateItem();
			updateGameInfo(event);
		}

		for (int i = 0; i < player.size(); i++) {
			if (player[i].getDirection() != "null") {
				player[i].processDirection(player[i].getDirectionCode());
			}
		}
}

void tEvent::addInfo() {
	for (int i = 0; i < 2; i++) {
		if (checkKeystroke(this->value, i)) {
			this->position = player[i].position;
			this->owner = player[i].id;
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

	//create sfml window
	app.create(sf::VideoMode(mapData.col * cellWidth, mapData.row * cellHeight), "BoomOffline2Players - hieutm211", sf::Style::Close);
	app.setFramerateLimit(framerateLimit);
	app.setKeyRepeatEnabled(false);

	//display guide
	sf::Sprite guideSprite;
	sf::Texture guideTexture;
	if (!guideTexture.loadFromFile("images/guide.png", sf::IntRect(0, 0, mapData.col*cellWidth, mapData.row*cellHeight))) {
		cout << "err";
	}

	app.clear(sf::Color::White);

	guideSprite.setTexture(guideTexture);
	guideSprite.setPosition(sf::Vector2f(0, 0));

	app.draw(guideSprite);
	app.display();
	system("sleep 5");
	
	//background music
	music.play();

	//game loop
	while (app.isOpen()) {
		gameTick++;

		sf::Event sfEvent;
		tEvent inputEvent;
			if (mapData.stopCode != -1) {
				endGame(mapData.stopCode);
				return 0;
			}

		while (app.pollEvent(sfEvent)) {
			if (sfEvent.type == sf::Event::Closed) {
				app.close();
				return 0;
			}
			
			inputEvent.value = getKeyPress(sfEvent);

			if (inputEvent.value != "null") {
				inputEvent.type = "direction";
				inputEvent.addInfo();
				inputEvent.updateInfo();

				if (inputEvent.isValid()) {
					updateGameInfo(inputEvent);
				}
			}

			inputEvent.value = getDirectionReleased(sfEvent);

			if (inputEvent.value != "null") {
				inputEvent.type = "direction released";
				inputEvent.addInfo();
				inputEvent.updateInfo();
				updateGameInfo(inputEvent);
			}
		}

		updateGameStatus();
	}
	
    return 0;
}
