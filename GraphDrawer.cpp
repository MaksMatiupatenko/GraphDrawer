#define SFML_STATIC

#include <SFML/Graphics.hpp>
#include <fstream>
#include <vector>
#include <chrono>
#include <random>
#include <string>
#include <sstream>
#include <iostream>

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
		d /= 50;
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

		velocity -= (pos - boardSize / 2.0f) * force / 3.0f * time;

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
};

class Edge : public sf::Drawable {
private:
	Node* nd1 = nullptr;
	Node* nd2 = nullptr;

	sf::Color col;
	float size = 0;
	float optLen = 0;

public:
	void interact(float time) {
		float d0 = length(nd1->pos - nd2->pos);
		float d = d0 - optLen;
		nd1->velocity += ((nd2->pos - nd1->pos) / d0) * force * time * d;
		nd2->velocity += ((nd1->pos - nd2->pos) / d0) * force * time * d;
	}

	void draw(sf::RenderTarget& window, sf::RenderStates states) const {
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

	sf::Color getColor() {
		return col;
	}
};

class Graph : public sf::Drawable {
private:
	std::vector <Node*> node;
	std::vector <Edge*> edge;
	sf::Font font;

	std::vector <std::vector <std::string>> actions;
	std::vector <std::vector <std::string>> rActions;
	int curAction = 0;

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
		float scale = std::min(20.f / n, 1.f);
		for (int i = 0; i < n; ++i) {
			Node nd;

			nd.setPos({
				rnd01() * (boardSize.x - 2 * boardOffset.x) + boardOffset.x,
				rnd01() * (boardSize.y - 2 * boardOffset.y) + boardOffset.y });
			nd.setSize(26 * scale);
			nd.setFillColor({ 50, 50, 50 });
			nd.setOutlineSize(4 * scale);
			nd.setOutlineColor({ 255, 255, 255 });
			nd.setFont(graph.font);
			nd.setString(std::to_string(i + 1));

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
			eg.setColor({ 100, 100, 100 });

			graph.edge.push_back(new Edge(eg));
		}

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
};

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
	std::ifstream fin("GraphLog.txt");
	fin >> graph;

	Node* toMove = nullptr;
	sf::Vector2f toMovePos0;
	sf::Vector2f wasMousePos;

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
					toMove = graph.getNodeAtPoint({ (float)event.mouseButton.x, (float)event.mouseButton.y });
					if (toMove != nullptr) {
						toMovePos0 = toMove->getPos();
						wasMousePos = { (float)event.mouseButton.x, (float)event.mouseButton.y };
					}
				}
			}

			if (event.type == sf::Event::MouseButtonReleased) {
				if (event.mouseButton.button == sf::Mouse::Left) {
					if (toMove != nullptr) {
						toMove->clearVelocity();
					}
					toMove = nullptr;
				}
			}

			if (event.type == sf::Event::KeyPressed) {
				if (event.key.code == sf::Keyboard::Right) {
					graph.nextAction(fin);
				}
				if (event.key.code == sf::Keyboard::Left) {
					graph.prevAction();
				}
			}
		}

		graph.update(time);

		if (toMove != nullptr) {
			toMove->setPos(toMovePos0 - wasMousePos + sf::Vector2f(sf::Mouse::getPosition()));
			toMove->clearVelocity();
		}

		window.clear();
		window.draw(graph);
		window.display();
	}

	fin.close();
	return 0;
}
