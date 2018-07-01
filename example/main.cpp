#include "handle.h"
#include <string>

// Just a dummy texture class.
class Texture
{
public:
	Texture(const char* _path) { m_path = _path; }
private:
	std::string m_path;
};

int main(int argc, const char* argv[])
{
	// Let's declare a nice name for our new handle type.
	// The using is not mandatory, but it is convenient to keep the name short 
	// (especially if you don't use the default template param values).
	using TextureID = Handle<Texture>;

	// Now, let's create a texture. It will be allocated inside the Handle<Texture> pool,
	// which provides fast and almost contiguous allocations.
	TextureID helloTexID = TextureID::Create("hello_world.png");
	// At this point, if you break into Visual Studio's debugger, you should be able to see
	// the texture variable when inspecting helloTexID.
	HDL_ASSERT(helloTexID != TextureID::kInvalid);

	// To get the texture from the handle, just call get.
	Texture* helloTex = TextureID::Get(helloTexID);
	HDL_ASSERT(helloTex);

	// And now let's destroy it!
	TextureID::Destroy(helloTexID);

	// At this point, the texture was destroyed and the handle has become invalid,
	// it is not possible to get a pointer to the texture anymore.
	Texture* helloAgain = TextureID::Get(helloTexID);
	HDL_ASSERT(helloAgain == nullptr);

	// The value of helloTexID may be re-used later, but not until all the other possible handle values have been used first. 
	// In this example where TextureID is 32 bits, that's ~4 billions calls to TextureID::Create later, so that should be fine.
	// But you can use 64 bits handles if you want.
	for (int i = 0; i < 100; ++i)
	{
		auto id = TextureID::Create("test");
		HDL_ASSERT(id != helloTexID);
	}
}