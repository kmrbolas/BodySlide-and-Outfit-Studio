/*
BodySlide and Outfit Studio
Copyright (C) 2018  Caliente & ousnius
See the included LICENSE file
*/

#include "ObjFile.h"

ObjFile::ObjFile() {
	scale = Vector3(1.0f, 1.0f, 1.0f);
	uvDupThreshold = 0.005f;
}

ObjFile::~ObjFile() {
	for (auto &d : data)
		delete d.second;
}

int ObjFile::AddGroup(const std::string& name, const std::vector<Vector3>& verts, const std::vector<Triangle>& tris, const std::vector<Vector2>& uvs) {
	if (name.empty() || verts.empty())
		return 1;

	ObjData* newData = new ObjData();
	newData->name = name;
	newData->verts = verts;
	newData->tris = tris;
	newData->uvs = uvs;

	data[name] = newData;
	return 0;
}

int ObjFile::LoadForNif(const std::string& fileName, const ObjOptionsImport& options) {
	std::fstream base(fileName.c_str(), std::ios_base::in | std::ios_base::binary);
	if (base.fail())
		return 1;

	LoadForNif(base, options);
	base.close();
	return 0;
}

int ObjFile::LoadForNif(std::fstream& base, const ObjOptionsImport& options) {
	ObjData* di = new ObjData();

	Vector3 v;
	Vector2 uv;
	Vector2 uv2;
	Vector3 vn;
	Vector3 vn2;
	Triangle t;

	std::string dump;
	std::string curgrp;
	std::string facept1;
	std::string facept2;
	std::string facept3;
	std::string facept4;
	int f[4];
	int ft[4];
	int fn[4];
	int nPoints = 0;
	int v_idx[4];

	std::vector<Vector3> verts;
	std::vector<Vector2> uvs;
	std::vector<Vector3> norms;
	size_t pos;

	std::map<int, std::vector<IndexMap>> vertUVMap;
	std::map<int, std::vector<IndexMap>> vertNormMap;

	bool gotface = false;

	while (!base.eof()) {
		if (!gotface)
			base >> dump;
		else
			gotface = false;

		if (options.noFaces) {
			if (dump.compare("v") == 0) {
				base >> v.x >> v.y >> v.z;
				di->verts.push_back(v);
			}
			else if (dump.compare("o") == 0 || dump.compare("g") == 0) {
				base >> curgrp;

				if (!di->name.empty()) {
					data[di->name] = di;
					di = new ObjData;
				}

				di->name = curgrp;
			}
			else if (dump.compare("vt") == 0) {
				base >> uv.u >> uv.v;
				uv.v = 1.0f - uv.v;
				di->uvs.push_back(uv);
			}

			continue;
		}

		if (dump.compare("v") == 0) {
			base >> v.x >> v.y >> v.z;
			verts.push_back(v);
		}
		else if (dump.compare("o") == 0 || dump.compare("g") == 0) {
			base >> curgrp;

			if (!di->name.empty()) {
				data[di->name] = di;
				di = new ObjData;
			}

			di->name = curgrp;
		}
		else if (dump.compare("vt") == 0) {
			base >> uv.u >> uv.v;
			uv.v = 1.0f - uv.v;
			uvs.push_back(uv);
		}
		else if (dump.compare("vn") == 0) {
			base >> vn.x >> vn.y >> vn.z;
			norms.push_back(vn);
		}
		else if (dump.compare("f") == 0) {
			base >> facept1 >> facept2 >> facept3;

			f[0] = atoi(facept1.c_str()) - 1;
			pos = facept1.find('/');
			ft[0] = atoi(facept1.substr(pos + 1).c_str()) - 1;
			pos = facept1.find('/', pos + 1);
			fn[0] = atoi(facept1.substr(pos + 1).c_str()) - 1;

			f[1] = atoi(facept2.c_str()) - 1;
			pos = facept2.find('/');
			ft[1] = atoi(facept2.substr(pos + 1).c_str()) - 1;
			pos = facept2.find('/', pos + 1);
			fn[1] = atoi(facept2.substr(pos + 1).c_str()) - 1;

			f[2] = atoi(facept3.c_str()) - 1;
			pos = facept3.find('/');
			ft[2] = atoi(facept3.substr(pos + 1).c_str()) - 1;
			pos = facept3.find('/', pos + 1);
			fn[2] = atoi(facept3.substr(pos + 1).c_str()) - 1;

			if (f[0] == -1 || f[1] == -1 || f[2] == -1)
				continue;

			base >> facept4;
			dump = facept4;

			nPoints = 3;
			gotface = true;

			if (facept4.length() > 0) {
				f[3] = atoi(facept4.c_str()) - 1;

				if (f[3] != -1) {
					pos = facept4.find('/');
					ft[3] = atoi(facept4.substr(pos + 1).c_str()) - 1;
					pos = facept4.find('/', pos + 1);
					fn[3] = atoi(facept4.substr(pos + 1).c_str()) - 1;
					nPoints = 4;
					gotface = false;
				}
			}

			bool skipFace = false;
			for (int i = 0; i < nPoints; i++) {
				int reuseIndex = -1;
				v_idx[i] = di->verts.size();

				auto savedVertUV = vertUVMap.find(f[i]);
				if (savedVertUV != vertUVMap.end()) {
					int uvIndex = -1;
					for (int j = 0; j < savedVertUV->second.size(); j++) {
						if (savedVertUV->second[j].i == ft[i]) {
							uvIndex = savedVertUV->second[j].v;
							break;
						}
						else if (uvs.size() > 0) {
							uv = uvs[ft[i]];
							uv2 = uvs[savedVertUV->second[j].i];

							if (fabs(uv.u - uv2.u) <= uvDupThreshold && fabs(uv.v - uv2.v) <= uvDupThreshold) {
								uvIndex = savedVertUV->second[j].v;
								break;
							}
						}
					}

					reuseIndex = uvIndex;
				}

				if (reuseIndex != -1) {
					auto savedVertNorm = vertNormMap.find(f[i]);
					if (savedVertNorm != vertNormMap.end()) {
						for (int j = 0; j < savedVertNorm->second.size(); j++) {
							if (norms.size() > 0) {
								vn = norms[fn[i]];
								vn2 = norms[savedVertNorm->second[j].i];

								if (!(vn - vn2).IsZero(true)) {
									reuseIndex = -1;
									break;
								}
							}
						}
					}
				}

				if (reuseIndex != -1)
					v_idx[i] = reuseIndex;

				if (v_idx[i] == di->verts.size()) {
					if (verts.size() > f[i]) {
						di->verts.push_back(verts[f[i]]);

						if (uvs.size() > ft[i]) {
							vertUVMap[f[i]].push_back(IndexMap(v_idx[i], ft[i]));
							di->uvs.push_back(uvs[ft[i]]);
						}

						if (norms.size() > fn[i]) {
							vertNormMap[f[i]].push_back(IndexMap(v_idx[i], fn[i]));
							di->norms.push_back(norms[fn[i]]);
						}
					}
					else {
						skipFace = true;
						break;
					}
				}
			}

			if (skipFace)
				continue;

			t.p1 = v_idx[0];
			t.p2 = v_idx[1];
			t.p3 = v_idx[2];
			di->tris.push_back(t);
			if (nPoints == 4) {
				t.p1 = v_idx[0];
				t.p2 = v_idx[2];
				t.p3 = v_idx[3];
				di->tris.push_back(t);
			}
		}
	}

	if (di->name.empty())
		di->name = "object";

	data[di->name] = di;
	return 0;
}


int ObjFile::Save(const std::string &fileName) {
	std::ofstream file(fileName.c_str(), std::ios_base::binary);
	if (file.fail())
		return 1;

	file << "# Outfit Studio - OBJ Export" << std::endl;
	file << "# https://github.com/ousnius/BodySlide-and-Outfit-Studio" << std::endl << std::endl;

	size_t pointOffset = 0;

	for (auto& d : data) {
		file << "o " << d.first << std::endl;

		for (int i = 0; i < d.second->verts.size(); i++) {
			file << "v " << (d.second->verts[i].x + offset.x) * scale.x
				<< " " << (d.second->verts[i].y + offset.y) * scale.y
				<< " " << (d.second->verts[i].z + offset.z) * scale.z
				<< std::endl;
		}
		file << std::endl;

		for (int i = 0; i < d.second->uvs.size(); i++)
			file << "vt " << d.second->uvs[i].u << " " << (1.0f - d.second->uvs[i].v) << std::endl;
		file << std::endl;

		for (int i = 0; i < d.second->tris.size(); i++) {
			file << "f " << d.second->tris[i].p1 + pointOffset + 1 << " "
				<< d.second->tris[i].p2 + pointOffset + 1 << " "
				<< d.second->tris[i].p3 + pointOffset + 1
				<< std::endl;
		}
		file << std::endl;

		pointOffset += d.second->verts.size();
	}

	file.close();
	return 0;
}

bool ObjFile::CopyDataForGroup(const std::string &name, std::vector<Vector3> *v, std::vector<Triangle> *t, std::vector<Vector2> *uv, std::vector<Vector3>* norms) {
	if (data.find(name) == data.end())
		return false;

	ObjData* od = data[name];
	if (v) {
		v->clear();
		v->resize(od->verts.size());
		for (int i = 0; i < od->verts.size(); i++) {
			(*v)[i].x = (od->verts[i].x + offset.x) * scale.x;
			(*v)[i].y = (od->verts[i].y + offset.y) * scale.y;
			(*v)[i].z = (od->verts[i].z + offset.z) * scale.z;
		}
	}

	if (t) {
		t->clear();
		t->resize(od->tris.size());
		for (int i = 0; i < od->tris.size(); i++)
			(*t)[i] = od->tris[i];
	}

	if (uv) {
		uv->clear();
		uv->resize(od->uvs.size());
		for (int i = 0; i < od->uvs.size(); i++)
			(*uv)[i] = od->uvs[i];
	}

	if (norms) {
		norms->clear();
		norms->resize(od->norms.size());
		for (int i = 0; i < od->norms.size(); i++)
			(*norms)[i] = od->norms[i];
	}

	return true;
}

std::vector<std::string> ObjFile::GetGroupList() {
	std::vector<std::string> groupList;
	groupList.reserve(data.size());

	for (const auto& group : data)
		groupList.push_back(group.first);

	return groupList;
}
