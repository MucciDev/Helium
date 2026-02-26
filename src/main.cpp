#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/ShaderLayer.hpp>
#include <Geode/modify/CCFileUtils.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/CCParticleSystemQuad.hpp>
#include <Geode/modify/CCLabelBMFont.hpp>
#include <Geode/modify/CCSprite.hpp>
#include <Geode/modify/CCTextureCache.hpp>

#include <chrono>
#include <unordered_map>
#include <string>
#include <string_view>

#ifdef GEODE_IS_WINDOWS
#include <windows.h>
#endif

using namespace geode::prelude;

// ==========================================
// CORE STATE
// ==========================================
std::chrono::steady_clock::time_point g_bootStartTime;

// ==========================================
// ALLOCATION-FREE STRING FORMATTING
// ==========================================
bool fastFormatHook(cocos2d::CCString* self, const char* format, va_list ap) {
    thread_local char stackBuffer[4096];
    
    va_list ap_copy;
    va_copy(ap_copy, ap);
    int len = std::vsnprintf(stackBuffer, sizeof(stackBuffer), format, ap_copy);
    va_end(ap_copy);
    
    if (len >= 0 && len < sizeof(stackBuffer)) {
        self->m_sString.assign(stackBuffer, len);
        return true;
    }
    
    std::string str(len + 1, '\0');
    std::vsnprintf(str.data(), str.size(), format, ap);
    str.resize(len); 
    self->m_sString = str;
    return true;
}

$execute {
    g_bootStartTime = std::chrono::steady_clock::now();
    
#ifdef GEODE_IS_WINDOWS
    auto addr = GetProcAddress(GetModuleHandleA("libcocos2d.dll"), "?initWithFormatAndValist@CCString@cocos2d@@AAE_NPBDPAD@Z");
    if (addr) {
        (void) Mod::get()->hook(
            reinterpret_cast<void*>(addr),
            &fastFormatHook,
            "cocos2d::CCString::initWithFormatAndValist",
            tulip::hook::TulipConvention::Thiscall
        );
    }
#endif
}

// ==========================================
// ULTRA-FAST I/O CACHE
// ==========================================
class $modify(OptimizedFileUtils, CCFileUtils) {
    gd::string fullPathForFilename(const char* pszFileName, bool bResolutionDirectory) {
        if (!pszFileName || pszFileName[0] == '\0') return "";
        
        thread_local std::unordered_map<std::string, gd::string> tls_pathCache;
        thread_local std::string tls_keyBuffer;
        
        tls_keyBuffer.clear();
        tls_keyBuffer.append(pszFileName);
        tls_keyBuffer.push_back(bResolutionDirectory ? '1' : '0');
        
        auto it = tls_pathCache.find(tls_keyBuffer);
        if (it != tls_pathCache.end()) return it->second;

        gd::string result = CCFileUtils::fullPathForFilename(pszFileName, bResolutionDirectory);
        tls_pathCache.emplace(tls_keyBuffer, result);
        return result;
    }
};

// ==========================================
// GPU DRAW CALL & OPACITY CULLING
// ==========================================
class $modify(OptimizedSprite, CCSprite) {
    void draw() {
        if (this->getOpacity() == 0 || !this->isVisible()) return;
        CCSprite::draw();
    }
    
    void setOpacity(GLubyte opacity) {
        if (this->getOpacity() == opacity) return;
        CCSprite::setOpacity(opacity);
    }
};

// ==========================================
// GEOMETRY REBUILD CULLING
// ==========================================
class $modify(OptimizedLabel, CCLabelBMFont) {
    void setString(const char* newString) {
        if (newString && this->getString()) {
            if (std::string_view(this->getString()) == newString) {
                return;
            }
        }
        CCLabelBMFont::setString(newString);
    }
};

// ==========================================
// PHYSICS CULLING
// ==========================================
class $modify(OptimizedParticles, CCParticleSystemQuad) {
    void update(float dt) {
        if (this->getParticleCount() == 0 || !this->isVisible() || this->getOpacity() == 0) return;
        CCParticleSystemQuad::update(dt);
    }
};

// ==========================================
// FAST BOOT & VRAM GARBAGE COLLECTION
// ==========================================
class $modify(FastBootLoadingLayer, LoadingLayer) {
    bool init(bool fromReload) {
        if (!LoadingLayer::init(fromReload)) return false;
        CCApplication::sharedApplication()->toggleVerticalSync(false);
        CCDirector::sharedDirector()->setAnimationInterval(0.0f);
        this->setVisible(false);
        return true;
    }
};

class $modify(FastBootMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        
        static bool s_firstBoot = true;
        if (s_firstBoot) {
            s_firstBoot = false;
            auto bootEndTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(bootEndTime - g_bootStartTime);
            log::info("[Helium] Engine ignited in {}ms", duration.count());

            cocos2d::CCTextureCache::sharedTextureCache()->removeUnusedTextures();
        }

        auto gm = GameManager::sharedState();
        if (gm->getGameVariable("0030")) CCApplication::sharedApplication()->toggleVerticalSync(true);
        float targetFPS = gm->m_customFPSTarget == 0 ? 60.0f : gm->m_customFPSTarget;
        CCDirector::sharedDirector()->setAnimationInterval(1.0f / targetFPS);
        return true;
    }
};

// ==========================================
// SHADER CACHE
// ==========================================
class $modify(OptimizedShaderLayer, ShaderLayer) {
    void setupShader(bool shouldReset) {
        if (!shouldReset && this->m_shader) return; 
        ShaderLayer::setupShader(shouldReset);
    }
};

// ==========================================
// OS HARDWARE PRIORITY
// ==========================================
$on_mod(Loaded) {
#ifdef GEODE_IS_WINDOWS
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetProcessPriorityBoost(GetCurrentProcess(), FALSE); 
#endif
}

