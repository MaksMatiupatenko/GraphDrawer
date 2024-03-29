﻿#define SFML_STATIC

#include <SFML/Graphics.hpp>
#include <fstream>
#include <vector>
#include <chrono>
#include <random>
#include <string>
#include <sstream>
#include <iostream>
#include <set>
#include <algorithm>
#include <filesystem>

std::mt19937 rnd(std::chrono::high_resolution_clock::now().time_since_epoch().count());
float rnd01() {
	return (double)rnd() / rnd.max();
}

const float force = 3;

sf::Vector2f boardSize;
sf::Vector2f boardOffset;

float length(const sf::Vector2f& vec) {
	return sqrt(vec.x * vec.x + vec.y * vec.y);
}

class Node : public sf::Drawable {
private:
	sf::Vector2f pos;
	sf::Color fillCol;
	sf::Color outlineCol;
	float size = 0;
	float outlineSize = 0;
	sf::Vector2f velocity;
	sf::Font font;
	std::string str;
	float scale = 1;

	friend class Edge;

public:
	void draw(sf::RenderTarget& window, sf::RenderStates states) const {
		sf::CircleShape shape;
		shape.setPosition(pos);
		shape.setRadius(size);
		shape.setOutlineThickness(outlineSize);
		shape.setFillColor(fillCol);
		shape.setOutlineColor(outlineCol);
		shape.setOrigin({ size, size });

		window.draw(shape, states);

		if (!str.empty()) {
			sf::Text text;
			text.setFont(font);
			text.setString(str);
			text.setCharacterSize(20);
			text.setOutlineThickness(2);
			text.setFillColor({ 255, 255, 255 });
			text.setOutlineColor({ 20, 20, 20 });
			
			float sz = std::max(text.getGlobalBounds().width, text.getGlobalBounds().height);
			if (sz > 0) {
				text.scale({ size * 1.3f / sz, size * 1.3f / sz });
				text.setPosition(pos - sf::Vector2f(text.getGlobalBounds().width, text.getGlobalBounds().height) / 2.0f);
				window.draw(text);
			}
		}
	}

	void interact(Node& other, float time) {
		float d = length(pos - other.pos);
		d /= scale;
		d /= 100;
		if (d > 0) {
			velocity += (pos - other.pos) / d * (force / d) * time;
			other.velocity += (other.pos - pos) / d * (force / d) * time;
		}
	}

	void update(float time) {
		if (pos.x < boardOffset.x) {
			velocity.x += (boardOffset.x - pos.x) * (boardOffset.x - pos.x) * force * time * 4.f;
		}
		if (pos.x > boardSize.x - boardOffset.x) {
			velocity.x -= (pos.x - boardSize.x + boardOffset.x) * (pos.x - boardSize.x + boardOffset.x) * force * time * 4.f;
		}
		if (pos.y < boardOffset.y) {
			velocity.y += (boardOffset.y - pos.y) * (boardOffset.y - pos.y) * force * time * 4.f;
		}
		if (pos.y > boardSize.y - boardOffset.y) {
			velocity.y -= (pos.y - boardSize.y + boardOffset.y) * (pos.y - boardSize.y + boardOffset.y) * force * time * 4.f;
		}

		velocity -= (pos - boardSize / 2.0f) / scale * force / 3.0f * time;

		pos += velocity * time;
		velocity *= exp(-time);
	}

	void setPos(const sf::Vector2f& vec) {
		pos = vec;
	}
	void setFillColor(const sf::Color& col) {
		fillCol = col;
	}
	void setOutlineColor(const sf::Color& col) {
		outlineCol = col;
	}
	void setSize(float sz) {
		size = sz;
	}
	void setOutlineSize(float sz) {
		outlineSize = sz;
	}
	void setFont(const sf::Font& _font) {
		font = _font;
	}
	void setString(const std::string& text) {
		str = text;
	}
	void setScale(float sc) {
		scale = sc;
	}

	bool isContains(const sf::Vector2f& point) {
		return length(point - pos) <= size + outlineSize;
	}

	sf::Vector2f getPos() {
		return pos;
	}
	sf::Color getColor() {
		return outlineCol;
	}

	void clearVelocity() {
		velocity = { 0.f, 0.f };
	}

	int getNum() {
		return std::stoi(str);
	}
};

class Edge : public sf::Drawable {
private:
	Node* nd1 = nullptr;
	Node* nd2 = nullptr;

	sf::Color col;
	float size = 3;
	float optLen = 0;
	bool alive = true;

public:
	void interact(float time) {
		if (alive) {
			float d0 = length(nd1->pos - nd2->pos);
			float d = d0 - optLen;
			nd1->velocity += ((nd2->pos - nd1->pos) / d0) * force * time * d;
			nd2->velocity += ((nd1->pos - nd2->pos) / d0) * force * time * d;
		}
	}

	void draw(sf::RenderTarget& window, sf::RenderStates states) const {
		if (alive) {
			sf::Vector2f vec = nd1->pos - nd2->pos;
			vec /= length(vec);
			vec = sf::Vector2f(-vec.y, vec.x);

			sf::ConvexShape shape;
			shape.setPointCount(4);
			shape.setPoint(0, nd1->pos + vec * size);
			shape.setPoint(1, nd1->pos - vec * size);
			shape.setPoint(2, nd2->pos - vec * size);
			shape.setPoint(3, nd2->pos + vec * size);
			shape.setFillColor(col);

			window.draw(shape, states);
		}
	}

	void setFirstNode(Node* nd) {
		nd1 = nd;
	}
	void setSecondNode(Node* nd) {
		nd2 = nd;
	}
	void setColor(const sf::Color& color) {
		col = color;
	}
	void setSize(float sz) {
		size = sz;
	}
	void setOptLen(float ln) {
		optLen = ln;
	}
	void setAlive(bool _alive) {
		alive = _alive;
	}
	Node* getFirstNode() {
		return nd1;
	}
	Node* getSecondNode() {
		return nd2;
	}

	sf::Color getColor() {
		return col;
	}
};

sf::Color eBaseCol(100, 100, 100);

class Graph : public sf::Drawable {
private:
	float scale = 1;

	std::vector <Node*> node;
	std::vector <Edge*> edge;
	std::set <std::pair <Node*, Node*>> st;
	sf::Font font;

	std::vector <std::vector <std::string>> actions;
	std::vector <std::vector <std::string>> rActions;
	int curAction = 0;
	int nodeCnt = 0;

	std::string rAction(const std::string& action) const {
		std::stringstream ss(action);
		std::string ans;

		std::string type;
		ss >> type;
		if (type == "nc") {
			int v;
			ss >> v;
			ans = "nc " + std::to_string(v) + ' ';
			ans += std::to_string(node[(long long)v - 1]->getColor().toInteger());
		}
		if (type == "ec") {
			int v;
			ss >> v;
			ans = "ec " + std::to_string(v) + ' ';
			ans += std::to_string(edge[(long long)v - 1]->getColor().toInteger());
		}
		if (type == "ea") {
			int u, v;
			ss >> u >> v;
			ans = "popEdge " + std::to_string(v) + ' ';
		}
		if (type == "ed") {
			int eg;
			ss >> eg;
			ans = "setAliveTrue " + std::to_string(eg) + ' ';
		}

		return ans;
	}

	void readActionGroup(std::istream& is) {
		int n;
		if (!(is >> n)) {
			return;
		}
		is.get();
		actions.push_back({});
		rActions.push_back({});
		for (int i = 0; i < n; ++i) {
			std::string s;
			getline(is, s);
			actions.back().push_back(s);
			rActions.back().push_back(rAction(s));
		}
	}

	void doAction(const std::string& action) {
		std::stringstream ss(action);

		std::string type;
		ss >> type;
		if (type == "nc") {
			int v;
			ss >> v;
			unsigned int col;
			ss >> col;
			node[(long long)v - 1]->setOutlineColor(sf::Color(col));
		}
		if (type == "ec") {
			int v;
			ss >> v;
			unsigned int col;
			ss >> col;
			edge[(long long)v - 1]->setColor(sf::Color(col));
		}
		if (type == "ea") {
			int u, v;
			ss >> u >> v;
			--u; --v;
			Edge eg;
			eg.setFirstNode(node[u]);
			eg.setSecondNode(node[v]);

			eg.setSize(3 * scale);
			eg.setOptLen(200 * scale);
			eg.setColor(eBaseCol);

			edge.push_back(new Edge(eg));
		}
		if (type == "ed") {
			int eg;
			ss >> eg;
			edge[(long long)eg - 1]->setAlive(false);
		}
		if (type == "popEdge") {
			edge.pop_back();
		}
		if (type == "setAliveTrue") {
			int eg;
			ss >> eg;
			edge[(long long)eg - 1]->setAlive(true);
		}
	}

public:
	~Graph() {
		clear();
	}

	void setFont(const sf::Font& _font) {
		font = _font;
	}

	void update(float time) {
		for (auto nd1 : node) {
			for (auto nd2 : node) {
				nd1->interact(*nd2, time);
			}
		}
		for (auto eg : edge) {
			eg->interact(time);
		}
		for (auto nd : node) {
			nd->update(time);
		}
	}

	void draw(sf::RenderTarget& window, sf::RenderStates states) const {
		for (auto eg : edge) {
			window.draw(*eg, states);
		}
		for (auto nd : node) {
			window.draw(*nd, states);
		}
	}

	friend std::istream& operator>>(std::istream& is, Graph& graph) {
		graph.clear();

		int n, m;
		is >> n >> m;
		float scale = 1.f;
		if (n != 0) {
			scale = std::min(20.f / n, 1.f);
		}
		//graph.scale = scale;
		for (int i = 0; i < n; ++i) {
			Node nd;

			nd.setPos({
				rnd01() * (boardSize.x - 2 * boardOffset.x) + boardOffset.x,
				rnd01() * (boardSize.y - 2 * boardOffset.y) + boardOffset.y });
			nd.setSize(std::max(26 * scale, 9.f));
			nd.setFillColor({ 50, 50, 50 });
			nd.setOutlineSize(std::max(4 * scale, 1.f));
			nd.setOutlineColor({ 255, 255, 255 });
			nd.setFont(graph.font);
			nd.setString(std::to_string(i + 1));
			nd.setScale(sqrt(scale));

			graph.node.push_back(new Node(nd));
		}
		for (int i = 0; i < m; ++i) {
			Edge eg;
			int u, v;
			is >> u >> v;
			--u; --v;
			eg.setFirstNode(graph.node[u]);
			eg.setSecondNode(graph.node[v]);

			eg.setSize(3 * scale);
			eg.setOptLen(200 * scale);
			eg.setColor(eBaseCol);

			graph.edge.push_back(new Edge(eg));
		}

		graph.nodeCnt = n;

		return is;
	}

	void clear() {
		for (auto nd : node) {
			delete nd;
		}
		for (auto eg : edge) {
			delete eg;
		}

		node.clear();
		edge.clear();
	}

	Node* getNodeAtPoint(const sf::Vector2f& point) const {
		for (auto nd : node) {
			if (nd->isContains(point)) {
				return nd;
			}
		}
		return nullptr;
	}
	int getNodeAtPointInd(const sf::Vector2f& point) const {
		for (int i = 0; i < node.size(); ++i) {
			if (node[i]->isContains(point)) {
				return i;
			}
		}
		return -1;
	}

	bool nextAction(std::istream& is) {
		if (curAction == actions.size()) {
			readActionGroup(is);
		}
		if (curAction < actions.size()) {
			for (const auto& s : actions[curAction]) {
				doAction(s);
			}
			++curAction;
			return true;
		}
		return false;
	}

	bool prevAction() {
		if (curAction > 0) {
			--curAction;
			for (const auto& s : rActions[curAction]) {
				doAction(s);
			}
			return true;
		}
		return false;
	}

	void addNode(const sf::Vector2f& pos) {
		Node nd;
		nd.setPos(pos);
		nd.setString(std::to_string(nodeCnt + 1));
		nd.setScale(1);
		nd.setSize(std::max(26.f, 9.f));
		nd.setFillColor({ 50, 50, 50 });
		nd.setOutlineSize(std::max(4.f, 1.f));
		nd.setOutlineColor({ 255, 255, 255 });
		nd.setFont(font);
		node.push_back(new Node(nd));
		++nodeCnt;
	}

	void deleteNode(int ind) {
		if (ind < 0 || ind >= node.size()) return;

		std::vector <std::pair <Node*, Node*>> del1;
		std::vector <int> deli;
		for (int i = edge.size() - 1; i >= 0; --i) {
			if (edge[i]->getFirstNode() == node[ind] || edge[i]->getSecondNode() == node[ind]) {
				deli.push_back(i);
			}
		}
		for (auto [f, s] : st) {
			if (f == node[ind] || s == node[ind]) {
				del1.push_back({ f, s });
			}
		}

		for (auto i : deli) {
			delete edge[i];
			edge.erase(edge.begin() + i);
		}
		for (auto [f, s] : del1) {
			st.erase({ f, s });
		}

		delete node[ind];
		node.erase(node.begin() + ind);
	}

	void swapEdge(Node* nd1, Node* nd2) {
		if (st.count({ nd1, nd2 }) || st.count({ nd2, nd1 })) {
			st.erase({ nd1, nd2 });
			st.erase({ nd2, nd1 });
			for (int i = 0; i < edge.size(); ++i) {
				if (edge[i]->getFirstNode() == nd1 && edge[i]->getSecondNode() == nd2 || edge[i]->getFirstNode() == nd2 && edge[i]->getSecondNode() == nd1) {
					edge.erase(edge.begin() + i);
				}
			}
		}
		else if (nd1 != nd2) {
			Edge* eg = new Edge;
			eg->setFirstNode(nd1);
			eg->setSecondNode(nd2);
			eg->setColor(eBaseCol);
			eg->setOptLen(200);
			eg->setSize(3);
			edge.push_back(eg);
			st.insert({ nd1, nd2 });
		}
	}

	void renum() {
		std::vector <int> nums;
		for (auto nd : node) {
			nums.push_back(nd->getNum());
		}
		std::sort(nums.begin(), nums.end());
		nums.resize(std::unique(nums.begin(), nums.end()) - nums.begin());
		for (auto nd : node) {
			nd->setString(std::to_string(std::lower_bound(nums.begin(), nums.end(), nd->getNum()) - nums.begin() + 1));
		}
		nodeCnt = node.size();
	}

	std::string toString() const {
		std::vector <int> nums;
		for (auto nd : node) {
			nums.push_back(nd->getNum());
		}
		std::sort(nums.begin(), nums.end());
		nums.resize(std::unique(nums.begin(), nums.end()) - nums.begin());

		std::string str;
		str = std::to_string(node.size()) + " " + std::to_string(edge.size()) + "\n";
		for (auto eg : edge) {
			str += std::to_string(std::lower_bound(nums.begin(), nums.end(), eg->getFirstNode()->getNum()) - nums.begin() + 1);
			str += " ";
			str += std::to_string(std::lower_bound(nums.begin(), nums.end(), eg->getSecondNode()->getNum()) - nums.begin() + 1);
			str += "\n";
		}

		return str;
	}
};

class SavesMenu {
private:
	std::vector <std::string> paths;
	int curPos = 0;
	sf::Font font;

public:
	void reload() {
		paths.clear();
		std::filesystem::path path(std::filesystem::current_path().string() + "\\saves\\");
		for (const auto& dir : std::filesystem::directory_iterator(path)) {
			std::string s = dir.path().filename().string();
			if (s.substr(s.size() - 4) == ".txt") {
				paths.push_back(s.substr(0, s.size() - 4));
			}
		}
		sort(paths.begin(), paths.end());
	}

	SavesMenu() {
		reload();
	}

	void movePos(int val) {
		curPos += val;
		curPos = std::max(std::min(curPos, (int)paths.size()), 0);
	}

	void setFont(const sf::Font& _font) {
		font = _font;
	}

	void draw(sf::RenderWindow& window) {
		sf::Text text;
		text.setFont(font);
		std::string str;
		for (int i = -2; i <= std::min(2, (int)paths.size() - curPos - 1); ++i) {
			if (i == 0) {
				str += "> " + paths[curPos];
			}
			else if (curPos + i >= 0) {
				str += paths[curPos + i];
			}
			str += '\n';
		}
		text.setString(str);
		text.setFillColor({ 255, 255, 255 });
		text.setCharacterSize(30);
		text.setOutlineThickness(2);
		text.setOutlineColor({ 50, 50, 50 });
		window.draw(text);
	}

	void loadGraph(Graph& graph) {
		if (!paths.empty()) {
			std::ifstream fin("saves\\" + paths[curPos] + ".txt");
			fin >> graph;
		}
	}

	void saveGraph(const Graph& graph, const std::string& saveName) {
		std::ofstream fout("saves\\" + saveName + ".txt");
		fout << graph.toString();
	}
};

std::string helpString() {
	std::string s;
	s += "Controls:\n";
	s += "    Help: H\n";
	s += "    Normalize graph numeration: R\n";
	s += "    Exit: Alt + F4\n";
	s += "    Loading menu:\n";
	s += "        Open/close: M\n";
	s += "        Move: Up/Down arrows\n";
	s += "        Load save: Enter\n";
	s += "    Save graph:\n";
	s += "	      Ctrl + S, than type save name\n";
	s += "	      Save: Enter\n";
	s += "	      Exit: Escape\n";
	s += "    Actions with graph:\n";
	s += "	      Move node:\n";
	s += "	          Activate: 1\n";
	s += "	          Move nodes with mouse\n";
	s += "	      Add node:\n";
	s += "	          Activate: 2\n";
	s += "	          Add node by mouse click\n";
	s += "	      Delete node:\n";
	s += "	          Activate: 3\n";
	s += "	          Delete node by mouse click\n";
	s += "	      Switch edge:\n";
	s += "	          Activate: 4\n";
	s += "	          Drag edge with mouse\n";
	s += "	          If edge exists, it will be deleted\n";
	s += "	          Else, it will be created\n";
	return s;
}

int main() {
	sf::ContextSettings settings;
	settings.antialiasingLevel = 8;
	sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "window", sf::Style::Fullscreen, settings);
	boardSize = sf::Vector2f(window.getSize());
	boardOffset = boardSize / 10.f;

	sf::Font font;
	font.loadFromFile("font/arialmt.ttf");

	Graph graph;
	graph.setFont(font);
	//std::ifstream fin("GraphLog.txt");
	//fin >> graph;

	Node* toMove = nullptr;
	sf::Vector2f toMovePos0;
	sf::Vector2f wasMousePos;

	Node* subEdgeStart = nullptr;

	if (!std::filesystem::exists(std::filesystem::current_path().string() + "\\saves\\")) {
		std::filesystem::create_directory(std::filesystem::current_path().string() + "\\saves\\");
	}



	int actionType = 2;

	
	SavesMenu menu;
	menu.setFont(font);
	bool menuActive = false;

	std::string inpStr;
	bool inpActive = false;

	bool help = false;
	

	sf::Clock clock;
	while (window.isOpen()) {
		float time = clock.restart().asSeconds();

		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				window.close();
			}
			
			if (event.type == sf::Event::MouseButtonPressed) {
				if (event.mouseButton.button == sf::Mouse::Left) {
					if (actionType == 1) {
						toMove = graph.getNodeAtPoint({ (float)event.mouseButton.x, (float)event.mouseButton.y });
						if (toMove != nullptr) {
							toMovePos0 = toMove->getPos();
							wasMousePos = { (float)event.mouseButton.x, (float)event.mouseButton.y };
						}
					}
					if (actionType == 2) {
						graph.addNode({ (float)event.mouseButton.x, (float)event.mouseButton.y });
					}
					if (actionType == 3) {
						int ind = graph.getNodeAtPointInd(sf::Vector2f({ (float)event.mouseButton.x, (float)event.mouseButton.y }));
						graph.deleteNode(ind);
					}
					if (actionType == 4) {
						subEdgeStart = graph.getNodeAtPoint({ (float)event.mouseButton.x, (float)event.mouseButton.y });
					}
				}
			}
			if (event.type == sf::Event::MouseButtonReleased) {
				if (event.mouseButton.button == sf::Mouse::Left) {
					if (toMove != nullptr) {
						toMove->clearVelocity();
						toMove = nullptr;
					}
					if (subEdgeStart != nullptr) {
						Node* nd = graph.getNodeAtPoint({ (float)event.mouseButton.x, (float)event.mouseButton.y });
						if (nd != nullptr) {
							graph.swapEdge(subEdgeStart, nd);
						}
						subEdgeStart = nullptr;
					}
				}
			}
			
			if (event.type == sf::Event::TextEntered) {
				if (inpActive) {
					if ((char)event.text.unicode == '\b') {
						if (!inpStr.empty()) {
							inpStr.pop_back();
						}
					}
					else {
						inpStr += (char)event.text.unicode;
					}
				}
			}

			if (event.type == sf::Event::KeyPressed) {
				/*if (event.key.code == sf::Keyboard::Right) {
					graph.nextAction(fin);
				}
				if (event.key.code == sf::Keyboard::Left) {
					graph.prevAction();
				}*/
				if (event.key.code == sf::Keyboard::Num1 && !inpActive) {
					actionType = 1;
				}
				if (event.key.code == sf::Keyboard::Num2 && !inpActive) {
					actionType = 2;
				}
				if (event.key.code == sf::Keyboard::Num3 && !inpActive) {
					actionType = 3;
				}
				if (event.key.code == sf::Keyboard::Num4 && !inpActive) {
					actionType = 4;
				}
				if (event.key.code == sf::Keyboard::F12 && !inpActive) {
					sf::Texture tex;
					tex.create(window.getSize().x, window.getSize().y);
					tex.update(window);
					sf::Image im = tex.copyToImage();
					im.saveToFile("iovsivosnv.png");
				}
				if (event.key.code == sf::Keyboard::R && !inpActive) {
					graph.renum();
				}
				if (event.key.code == sf::Keyboard::M && !inpActive) {
					menuActive ^= 1;
				}
				if (event.key.code == sf::Keyboard::Up && !inpActive) {
					if (menuActive) {
						menu.movePos(-1);
					}
				}
				if (event.key.code == sf::Keyboard::Down && !inpActive) {
					if (menuActive) {
						menu.movePos(1);
					}
				}
				if (event.key.code == sf::Keyboard::Enter) {
					if (inpActive) {
						inpActive = false;

						menu.saveGraph(graph, inpStr);
						menu.reload();

						inpStr.clear();
					}
					else if (menuActive) {
						menu.loadGraph(graph);
					}
				}
				if (event.key.code == sf::Keyboard::Escape) {
					if (inpActive) {
						inpActive = false;
						inpStr.clear();
					}
				}
				if (event.key.code == sf::Keyboard::S && !inpActive) {
					if (event.key.control) {
						inpActive = true;
						inpStr = "save";
						while (window.pollEvent(event)) {}
					}
				}
				if (event.key.code == sf::Keyboard::H && !inpActive) {
					help ^= 1;
				}
			}
		}

		graph.update(time);

		if (toMove != nullptr) {
			toMove->setPos(toMovePos0 - wasMousePos + sf::Vector2f(sf::Mouse::getPosition()));
			toMove->clearVelocity();
		}

		window.clear(sf::Color(0, 0, 0, 0));
		if (subEdgeStart != nullptr) {
			Edge edge;
			edge.setColor(eBaseCol);
			edge.setFirstNode(subEdgeStart);
			Node nd;
			nd.setPos(sf::Vector2f(sf::Mouse::getPosition()));
			edge.setSecondNode(&nd);
			window.draw(edge);
		}
		window.draw(graph);

		if (menuActive) {
			menu.draw(window);
		}
		else {
			sf::Text text;
			text.setFont(font);
			if (actionType == 1) {
				text.setString("cur: move node");
			}
			if (actionType == 2) {
				text.setString("cur: add node");
			}
			if (actionType == 3) {
				text.setString("cur: delete node");
			}
			if (actionType == 4) {
				text.setString("cur: switch edge");
			}
			text.setFillColor({ 255, 255, 255 });
			text.setCharacterSize(30);
			text.setOutlineThickness(2);
			text.setOutlineColor({ 50, 50, 50 });
			window.draw(text);
		}

		{ //controls
			sf::Text text;
			text.setFont(font);
			if (help) {
				text.setString(helpString());
			}
			else {
				text.setString("Controls:\n    Help: H");
			}
			text.setPosition(window.getSize().x * 2.f / 3, 0);
			text.setFillColor({ 255, 255, 255 });
			text.setCharacterSize(30);
			text.setOutlineThickness(2);
			text.setOutlineColor({ 50, 50, 50 });
			window.draw(text);
		}

		if (inpActive) {
			sf::Text text;
			text.setFont(font);
			text.setString(inpStr);
			text.setPosition(window.getSize().x / 2.f - text.getGlobalBounds().width / 2.f, window.getSize().y / 2.f - text.getGlobalBounds().height / 2.f);
			text.setFillColor({ 255, 255, 255 });
			text.setCharacterSize(30);
			text.setOutlineThickness(2);
			text.setOutlineColor({ 50, 50, 50 });
			window.draw(text);
		}

		window.display();
	}

	//fin.close();
	return 0;
}
