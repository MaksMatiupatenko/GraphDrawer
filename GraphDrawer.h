#include <fstream>
#include <vector>
#include <map>
#include <iostream>

namespace gdraw {
	std::ofstream fout("C:\\Users\\Galina\\Desktop\\прг\\что-то\\GraphDrawer\\ivan\\GraphLog.txt");

	int actCnt = 1;
	std::vector <std::string> buff;
	std::map <std::pair <int, int>, int> edgeNum;

	void init(int nodeCnt, const std::vector <std::pair <int, int>>& edge) {
		fout.clear();

		fout << nodeCnt << ' ' << edge.size() << '\n';
		for (int i = 0; i < edge.size(); ++i) {
			auto [u, v] = edge[i];
			edgeNum[{u, v}] = i;
			edgeNum[{v, u}] = i;
			fout << u + 1 << ' ' << v + 1 << '\n';
		}
	}
	void init(const std::vector <std::vector <int>>& g) {
		std::vector <std::pair <int, int>> edge;
		for (int v = 0; v < g.size(); ++v) {
			for (auto u : g[v]) {
				if (u < v) {
					edge.push_back({ u, v });
				}
			}
		}
		init(g.size(), edge);
	}

	void flush() {
		if (!buff.empty()) {
			fout << buff.size() << '\n';
			for (const auto& s : buff) {
				fout << s << '\n';
			}
			buff.clear();
		}
	}

	void setBlockSize(int sz) {
		if (actCnt != 0) {
			std::cerr << "|ERROR| gdraw: prev block not finished\n";
		}
		else {
			actCnt = sz;
		}
	}

	void changeNodeColor(int node, unsigned int r, unsigned int g, unsigned int b) {
		buff.push_back("nc " + std::to_string(node + 1) + ' ' + std::to_string((r << 24) | (g << 16) | (b << 8) | 255));
		--actCnt;
		if (actCnt <= 0) {
			gdraw::flush();
		}
	}
	void clearNodeColor(int node) {
		buff.push_back("nc " + std::to_string(node + 1) + ' ' + std::to_string((255 << 24) | (255 << 16) | (255 << 8) | 255));
		--actCnt;
		if (actCnt <= 0) {
			gdraw::flush();
		}
	}

	void changeEdgeColor(int edge, unsigned int r, unsigned int g, unsigned int b) {
		buff.push_back("ec " + std::to_string(edge + 1) + ' ' + std::to_string((r << 24) | (g << 16) | (b << 8) | 255));
		--actCnt;
		if (actCnt <= 0) {
			gdraw::flush();
		}
	}
	void clearEdgeColor(int edge) {
		buff.push_back("ec " + std::to_string(edge + 1) + ' ' + std::to_string((255 << 24) | (255 << 16) | (255 << 8) | 255));
		--actCnt;
		if (actCnt <= 0) {
			gdraw::flush();
		}
	}
	void changeEdgeColor(int node1, int node2, unsigned int r, unsigned int g, unsigned int b) {
		changeEdgeColor(edgeNum[{node1, node2}], r, g, b);
	}
	void clearEdgeColor(int node1, int node2) {
		clearEdgeColor(edgeNum[{node1, node2}]);
	}
}
