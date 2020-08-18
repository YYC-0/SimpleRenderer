#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "../head/model.h"

Model::Model(const std::string& filename) : verts_(), faces_() {
    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) 
        {
            iss >> trash;
            Vector3f v;
            for (int i=0;i<3;i++) 
                iss >> v[i];
            verts_.push_back(v);
        } 
        else if (!line.compare(0, 3, "vn "))
        {
            iss >> trash >> trash;
            Vector3f n;
            for (int i = 0; i < 3; i++)
                iss >> n[i];
            norms.push_back(n);
        }
        else if (!line.compare(0, 3, "vt "))
        {
            iss >> trash >> trash;
            Vector2f uv;
            for (int i = 0; i < 2; ++i)
                iss >> uv[i];
            uv_.push_back(uv);
        }
        else if (!line.compare(0, 2, "f ")) 
        {
            std::vector<Vector3i> f;
            Vector3i tmp;
            iss >> trash;
            while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2]) {
                for (int i = 0; i < 3; i++) tmp[i]--; // in wavefront obj all indices start at 1, not zero
                f.push_back(tmp);
            }
            faces_.push_back(f);
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# "  << faces_.size() << std::endl;
    load_texture(filename, "_diffuse.tga", diffusemap_);
}

Model::~Model() {
}

int Model::nverts() const
{
    return (int)verts_.size();
}

int Model::nfaces() const
{
    return (int)faces_.size();
}

std::vector<int> Model::face(int idx) const 
{
    std::vector<int> face;
    for (int i = 0; i < (int)faces_[idx].size(); ++i)
        face.push_back(faces_[idx][i][0]);
    return face;
}

void Model::load_texture(std::string fileName, const char *suffix, TGAImage &img)
{
    std::string texfile(fileName);
    size_t dot = texfile.find_last_of(".");
    if (dot != std::string::npos) {
        texfile = texfile.substr(0, dot) + std::string(suffix);
        std::cerr << "texture file " << texfile << " loading " << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
        img.flip_vertically();
    }
}

Vector3f Model::vert(int i) const 
{
    return verts_[i];
}

Vector2i Model::uv(int iface, int nvert)
{
    int idx = faces_[iface][nvert][1];
    return Vector2i(uv_[idx].x() * diffusemap_.get_width(), uv_[idx].y() * diffusemap_.get_height());
}

Color Model::diffuse(Vector2i uv)
{
    TGAColor c = diffusemap_.get(uv.x(), uv.y());
    return Color(c.r, c.g, c.b);
}

