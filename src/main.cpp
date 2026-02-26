#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/ShaderLayer.hpp>
#include <Geode/modify/CCFileUtils.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/CCNode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCParticleSystemQuad.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <Geode/modify/CCLabelBMFont.hpp>
#include <Geode/modify/CCSprite.hpp>
#include <Geode/modify/CCTextureCache.hpp>

#include <chrono>
#include <unordered_map>
#include <string>

#ifdef GEODE_IS_WINDOWS
#include <windows.h>
#endif

using namespace geode::prelude;

// ==========================================
// CORE STATE
// ==========================================
bool g_suspendSorting = false;
bool g_localLevelsLoaded = false;
std::chrono::steady_clock::time_point g_bootStartTime;

// ==========================================
// ALLOCATION-FREE STRING FORMATTING
// ==========================================
bool fastFormatHook(cocos2d::CCString* self, const char* format, va_list ap) {
    // 4KB thread-local buffer prevents heap allocations for 99.9% of UI strings.
    thread_local char stackBuffer[4096];
    
    va_list ap_copy;
    va_copy(ap_copy, ap);
    int len = std::vsnprintf(stackBuffer, sizeof(stackBuffer), format, ap_copy);
    va_end(ap_copy);
    
    // If it fits in the buffer, assign directly
    if (len >= 0 && len < sizeof(stackBuffer)) {
        self->m_sString.assign(stackBuffer, len);
        return true;
    }
    
    // Fallback for massive strings
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
// ULTRA-FAST I/O CACHE (NO FORMATTING)
// ==========================================
class $modify(OptimizedFileUtils, CCFileUtils) {
    gd::string fullPathForFilename(const char* pszFileName, bool bResolutionDirectory) {
        if (!pszFileName || pszFileName[0] == '\0') return "";
        
        thread_local std::unordered_map<std::string, gd::string> tls_pathCache;
        thread_local std::string tls_keyBuffer;
        
        // Reserve memory once. Clears do not deallocate capacity.
        // Bypassing snprintf entirely for raw string appends is faster.
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
// DEFERRED DATA LOADING
// ==========================================
class $modify(DeferredGameManager, GameManager) {
    void loadDataFromFile(gd::string const& filename) {
        if (filename == "CCLocalLevels.dat" && !g_localLevelsLoaded) return;
        GameManager::loadDataFromFile(filename);
    }
};

class $modify(DeferredCreatorLayer, CreatorLayer) {
    bool init() {
        if (!g_localLevelsLoaded) {
            g_localLevelsLoaded = true;
            GameManager::sharedState()->loadDataFromFile("CCLocalLevels.dat");
        }
        return CreatorLayer::init();
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
// REDUNDANT STATE CULLING
// ==========================================
class $modify(OptimizedNode, CCNode) {
    void setVisible(bool visible) {
        if (this->m_bVisible == visible) return;
        CCNode::setVisible(visible);
    }

    void setPosition(CCPoint const& pos) {
        if (this->m_obPosition.x == pos.x && this->m_obPosition.y == pos.y) return;
        CCNode::setPosition(pos);
    }

    void setRotation(float fRotation) {
        if (this->m_fRotationX == fRotation && this->m_fRotationY == fRotation) return;
        CCNode::setRotation(fRotation);
    }

    void setScale(float fScale) {
        if (this->m_fScaleX == fScale && this->m_fScaleY == fScale) return;
        CCNode::setScale(fScale);
    }

    void sortAllChildren() {
        if (g_suspendSorting) {
            this->m_bReorderChildDirty = true;
            return;
        }
        CCNode::sortAllChildren();
    }
};

// ==========================================
// GEOMETRY REBUILD CULLING
// ==========================================
class $modify(OptimizedLabel, CCLabelBMFont) {
    void setString(const char* newString) {
        // Safely grab the current string via the getter and compare
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
        // Use standard getters to avoid protected member compiler errors
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
        
        auto bootEndTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(bootEndTime - g_bootStartTime);
        log::info("[Helium] Engine ignited in {}ms", duration.count());

        // Dump useless boot textures from VRAM to clear up GPU overhead
        cocos2d::CCTextureCache::sharedTextureCache()->removeUnusedTextures();

        auto gm = GameManager::sharedState();
        if (gm->getGameVariable("0030")) CCApplication::sharedApplication()->toggleVerticalSync(true);
        float targetFPS = gm->m_customFPSTarget == 0 ? 60.0f : gm->m_customFPSTarget;
        CCDirector::sharedDirector()->setAnimationInterval(1.0f / targetFPS);
        return true;
    }
};

// ==========================================
// LEVEL LOAD OPTIMIZATION
// ==========================================
class $modify(OptimizedPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        g_suspendSorting = true;
        bool result = PlayLayer::init(level, useReplay, dontCreateObjects);
        g_suspendSorting = false;
        
        if (this->m_objectLayer) this->m_objectLayer->sortAllChildren();
        return result;
    }
};

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
    // Favor process for CPU time, disable dynamic priority throttling
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetProcessPriorityBoost(GetCurrentProcess(), FALSE); // TRUE actually disables the boost in Windows API
#endif
}
