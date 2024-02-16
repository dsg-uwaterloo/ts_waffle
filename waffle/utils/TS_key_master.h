//
// Created by Peter Pan on 2/11/2024.
//

#ifndef WAFFLE_TS_KEY_MASTER_H
#define WAFFLE_TS_KEY_MASTER_H


#include <vector>
#include <string>
#include <sstream>
#include <random>
#include <set>
#include <algorithm>

//generate Time Series key by item_id, user_id and timestamp
class KeyGenerationDecomposition {
private:
    std::string set_user_id(const std::string &new_user_id) {
        if (new_user_id.length() > USER_ID_LENGTH) {
            user_id = new_user_id.substr(0, USER_ID_LENGTH);
        } else {
            //pad with 0s
            user_id = std::string(USER_ID_LENGTH - new_user_id.length(), '0') + new_user_id;
        }
        return user_id;
    };

public:
    static const int USER_ID_LENGTH = 16;

    std::string item_id;
    std::string user_id;
    std::string timestamp;
    std::string key;

    KeyGenerationDecomposition(const std::string &item_id, const std::string &new_user_id, const std::string &timestamp)
            : item_id(item_id),timestamp(timestamp) {

        set_user_id(new_user_id);
        key = item_id + "@" + user_id + "@" + timestamp;
    };

    KeyGenerationDecomposition(const std::string &key)
            : key(key) {
        item_id = key.substr(0, key.find("@"));
        user_id = key.substr(key.find("@")+1, key.find("@", key.find("@")+1));
        timestamp = key.substr(key.find("@", key.find("@")+1)+1);
    };

    KeyGenerationDecomposition(const std::string &item_id, const std::string &new_user_id)
            : item_id(item_id), timestamp(current_time()){
        set_user_id(new_user_id);
        key = item_id + "@" + user_id;

    };
    std::string current_time() {
        time_t now = time(0);
        return std::to_string(now);
    };

    std::string get_key() {
        return key;
    };

    std::string get_item_id() {
        return item_id;
    };

    std::string get_user_id() {
        return user_id;
    };

    std::string get_timestamp() {
        return timestamp;
    };

};


class ItemIdGenerator {
private:
    static const std::vector<std::string> categories;
    static const std::vector<std::string> subcategories;
    static const std::vector<std::string> data_types;
    static std::mt19937 generator;
    static std::uniform_int_distribution<> distribution;
    // Generates a random integer within a range
    static int generate_random_int(int min, int max) {
        std::uniform_int_distribution<int> distribution(min, max);
        return distribution(generator);
    }

    // Generates a single item ID with a unique suffix
    static std::string generate_single_id(int categoryIndex, int subcategoryIndex, int index) {
        std::stringstream ss;
        // Append a random suffix to ensure uniqueness
        int randomSuffix = generate_random_int(100, 999);
        ss << categories[categoryIndex] << "_" << subcategories[subcategoryIndex] << "_" << index << "_" << randomSuffix<< "&" << data_types[distribution(generator)]<< "&";
        return ss.str();
    }

public:
    static std::vector<std::string> generate_item_ids(int no_item_ids) {
        std::set<std::string> unique_ids;
        std::vector<std::string> item_ids;

        int categoryIndex = 0;
        int subcategoryIndex = 0;
        int index = 0;

        while (unique_ids.size() < no_item_ids) {
            std::string id = generate_single_id(categoryIndex, subcategoryIndex, index++);
            if (unique_ids.find(id) == unique_ids.end()) { // Ensure ID is unique
                unique_ids.insert(id);
                item_ids.push_back(id);
            }

            // Increment indices to simulate variety
            subcategoryIndex++;
            if (subcategoryIndex >= subcategories.size()) {
                subcategoryIndex = 0;
                categoryIndex++;
                if (categoryIndex >= categories.size()) {
                    categoryIndex = 0; // Reset to ensure we can generate more IDs if needed
                }
            }
        }

        return std::vector<std::string>(item_ids.begin(), item_ids.end());
    }
};

std::mt19937 ItemIdGenerator::generator = std::mt19937(std::random_device{}());
const std::vector<std::string> ItemIdGenerator::data_types = {"int", "float", "bool"};//"string", "bool", "blob"};
std::uniform_int_distribution<> ItemIdGenerator::distribution=std::uniform_int_distribution<>(0, ItemIdGenerator::data_types.size() - 1);
const std::vector<std::string> ItemIdGenerator::categories = {
        "CPU", "GPU", "DISK", "MEM", "NET", "IO", "SENSOR", "POWER", "BATTERY", "TEMP",
        "STORAGE", "DATABASE", "THREAD", "PROCESS", "FILESYSTEM", "SECURITY", "APPLICATION",
        "OS", "VIRTUALIZATION", "CLOUD", "CONTAINER", "NETWORK", "SESSION", "USER", "DEVICE",
        "ENVIRONMENT", "LOG", "EVENT", "ALERT", "PERIPHERAL", "AUDIO", "VIDEO", "GRAPHICS",
        "FRAMEWORK", "LIBRARY", "API", "QUEUE", "CACHE", "POOL", "ALLOCATOR", "HANDLER",
        "REQUEST", "RESPONSE", "TRANSACTION", "CONNECTION", "PROTOCOL", "SERVICE", "DAEMON",
        "MODULE", "PLUGIN", "EXTENSION", "INTERFACE", "HARDWARE", "SOFTWARE", "FIRMWARE",
        "DRIVER", "KERNEL", "SYSTEM", "CONFIGURATION", "OPTIMIZATION", "PERFORMANCE", "DIAGNOSTIC",
        "MONITORING", "LOGGING", "DEBUGGING", "TESTING", "DEPLOYMENT", "OPERATION", "MAINTENANCE",
        "UPGRADE", "PATCH", "RELEASE", "VERSION", "BUILD", "INSTALLATION", "ACTIVATION", "REGISTRATION",
        "LICENSE", "CERTIFICATE", "ENCRYPTION", "AUTHENTICATION", "AUTHORIZATION", "COMPLIANCE",
        "AUDIT", "POLICY", "STANDARD", "GUIDELINE", "PROCEDURE", "WORKFLOW", "TASK", "JOB",
        "PROJECT", "CAMPAIGN", "STRATEGY", "PLAN", "GOAL", "OBJECTIVE", "METRIC", "INDICATOR",
        "MEASUREMENT", "ANALYSIS", "REPORT", "DASHBOARD", "VISUALIZATION", "PREDICTION", "FORECAST",
        "SIMULATION", "MODELING", "OPTIMIZATION", "AUTOMATION", "INTEGRATION", "SYNCHRONIZATION",
        "COLLABORATION", "COMMUNICATION", "SOCIAL", "MOBILE", "WEB", "CLOUD", "ENTERPRISE", "CONSUMER",
        "INDUSTRIAL", "IOT", "AI", "ML", "DATA", "ANALYTICS", "BLOCKCHAIN", "CRYPTO", "VIRTUAL", "AUGMENTED",
        "REALITY", "GAMING", "STREAMING", "BROADCASTING", "PUBLISHING", "CONTENT", "MEDIA", "ADVERTISING",
        "MARKETING", "SALES", "COMMERCE", "FINANCE", "BANKING", "INSURANCE", "INVESTMENT", "TRADING",
        "ACCOUNTING", "TAXATION", "LEGAL", "REGULATION", "GOVERNANCE", "RISK", "SECURITY", "SAFETY",
        "HEALTH", "WELLNESS", "MEDICAL", "PHARMACEUTICAL", "BIOTECH", "RESEARCH", "DEVELOPMENT",
        "EDUCATION", "TRAINING", "SKILLS", "CAREER", "EMPLOYMENT", "RECRUITMENT", "HR", "MANAGEMENT",
        "LEADERSHIP", "ORGANIZATION", "CULTURE", "DIVERSITY", "EQUITY", "INCLUSION", "SUSTAINABILITY",
        "ENVIRONMENTAL", "SOCIAL", "GOVERNANCE", "ETHICS", "PHILANTHROPY", "VOLUNTEERING", "COMMUNITY",
        "EVENTS", "WORKSHOPS", "SEMINARS", "CONFERENCES", "EXHIBITIONS", "FAIRS", "SHOWS", "PERFORMANCES",
        "ART", "DESIGN", "ARCHITECTURE", "ENGINEERING", "CONSTRUCTION", "MANUFACTURING", "PRODUCTION",
        "LOGISTICS", "SUPPLY", "CHAIN", "TRANSPORTATION", "DELIVERY", "WAREHOUSING", "INVENTORY",
        "PROCUREMENT", "PURCHASING", "VENDING", "RETAIL", "WHOLESALE", "E-COMMERCE", "CUSTOMER",
        "SERVICE", "SUPPORT", "EXPERIENCE", "SATISFACTION", "LOYALTY", "ENGAGEMENT", "FEEDBACK",
        "REVIEW", "RATING", "SURVEY", "QUESTIONNAIRE", "INTERVIEW", "FOCUS", "GROUP", "STUDY",
        "RESEARCH", "INSIGHT", "INNOVATION", "CREATIVITY", "IDEATION", "PROTOTYPING", "TESTING",
        "LAUNCH", "SCALE", "GROWTH", "EXPANSION", "DIVERSIFICATION", "ADAPTATION", "TRANSFORMATION",
        "TURNAROUND", "RECOVERY", "CRISIS", "MANAGEMENT", "CONTINUITY", "RESILIENCE", "RELIABILITY",
        "QUALITY", "EXCELLENCE", "EFFICIENCY", "PRODUCTIVITY", "EFFECTIVENESS", "VALUE", "IMPACT",
        "CONTRIBUTION", "SUCCESS", "ACHIEVEMENT", "RECOGNITION", "AWARD", "HONOR", "PRESTIGE",
        "REPUTATION", "BRAND", "IMAGE", "IDENTITY", "AWARENESS", "PERCEPTION", "POSITIONING", "DIFFERENTIATION",
        "COMPETITIVE", "ADVANTAGE", "MARKET", "SHARE", "GROWTH", "PROFITABILITY", "REVENUE", "INCOME",
        "EARNINGS", "MARGIN", "COST", "EXPENSE", "INVESTMENT", "CAPITAL", "FUNDING", "FINANCING",
        "LENDING", "BORROWING", "CREDIT", "DEBT", "EQUITY", "ASSET", "LIABILITY", "VALUATION", "PRICING",
        "BUDGET", "FORECAST", "ALLOCATION", "RESOURCE", "MANAGEMENT", "PLANNING", "STRATEGY", "TACTIC",
        "OPERATION", "EXECUTION", "CONTROL", "MONITORING", "EVALUATION", "ASSESSMENT", "AUDIT",
        "REVIEW", "ANALYSIS", "DIAGNOSIS", "SOLUTION", "IMPROVEMENT", "OPTIMIZATION", "INNOVATION",
        "CHANGE", "DEVELOPMENT", "GROWTH", "LEARNING", "ADAPTATION", "EVOLUTION", "TRANSFORMATION",
        "REVOLUTION", "DISRUPTION", "BREAKTHROUGH", "ACHIEVEMENT", "SUCCESS", "EXCELLENCE", "LEADERSHIP",
        "VISION", "MISSION", "PURPOSE", "VALUE", "CULTURE", "ETHICS", "INTEGRITY", "TRANSPARENCY",
        "ACCOUNTABILITY", "RESPONSIBILITY", "COMMITMENT", "DEDICATION", "PASSION", "DRIVE", "MOTIVATION",
        "AMBITION", "ASPIRATION", "INSPIRATION", "CREATIVITY", "INNOVATION", "CURIOSITY", "INQUIRY",
        "EXPLORATION", "DISCOVERY", "LEARNING", "KNOWLEDGE", "WISDOM", "INSIGHT", "UNDERSTANDING",
        "AWARENESS", "PERCEPTION", "JUDGMENT", "DECISION", "CHOICE", "OPTION", "ALTERNATIVE", "OPPORTUNITY",
        "CHALLENGE", "PROBLEM", "ISSUE", "OBSTACLE", "BARRIER", "RISK", "THREAT", "VULNERABILITY",
        "WEAKNESS", "LIMITATION", "CONSTRAINT", "RESTRICTION", "COMPROMISE", "TRADE-OFF", "BALANCE",
        "EQUILIBRIUM", "HARMONY", "SYNERGY", "ALIGNMENT", "INTEGRATION", "COORDINATION", "COOPERATION",
        "COLLABORATION", "PARTNERSHIP", "ALLIANCE", "NETWORK", "COMMUNITY", "ECOSYSTEM", "ENVIRONMENT",
        "MARKET", "INDUSTRY", "SECTOR", "DOMAIN", "FIELD", "AREA", "NICHE", "SEGMENT", "CATEGORY",
        "CLASS", "TYPE", "KIND", "NATURE", "CHARACTER", "FEATURE", "ATTRIBUTE", "QUALITY", "PROPERTY",
        "ASPECT", "FACTOR", "ELEMENT", "COMPONENT", "PART", "PIECE", "UNIT", "MODULE", "SYSTEM",
        "STRUCTURE", "FRAMEWORK", "MODEL", "SCHEMA", "TEMPLATE", "PATTERN", "FORMULA", "RECIPE",
        "METHOD", "TECHNIQUE", "APPROACH", "STRATEGY", "TACTIC", "PLAN", "PROGRAM", "PROJECT",
        "CAMPAIGN", "INITIATIVE", "ACTION", "ACTIVITY", "TASK", "OPERATION", "PROCESS", "PROCEDURE",
        "PRACTICE", "ROUTINE", "HABIT", "CUSTOM", "TRADITION", "CONVENTION", "NORM", "STANDARD",
        "GUIDELINE", "POLICY", "REGULATION", "RULE", "LAW", "MANDATE", "ORDER", "DIRECTIVE", "INSTRUCTION",
        "REQUIREMENT", "SPECIFICATION", "CRITERION", "MEASURE", "INDICATOR", "METRIC", "PARAMETER",
        "VARIABLE", "CONSTANT", "FACTOR", "ELEMENT", "AGENT", "ACTOR", "PLAYER", "PARTICIPANT",
        "STAKEHOLDER", "INTEREST", "MOTIVE", "REASON", "CAUSE", "SOURCE", "ORIGIN", "ROOT", "BASIS",
        "FOUNDATION", "GROUND", "PRINCIPLE", "CONCEPT", "IDEA", "THOUGHT", "NOTION", "BELIEF", "OPINION",
        "VIEW", "PERSPECTIVE", "ANGLE", "POSITION", "STANDPOINT", "APPROACH", "METHOD", "MODE",
        "MEANS", "MEDIUM", "CHANNEL", "VEHICLE", "INSTRUMENT", "TOOL", "DEVICE", "APPLIANCE", "EQUIPMENT",
        "MACHINERY", "HARDWARE", "SOFTWARE", "TECHNOLOGY", "SYSTEM", "INFRASTRUCTURE", "FACILITY",
        "RESOURCE", "MATERIAL", "SUBSTANCE", "PRODUCT", "GOOD", "SERVICE", "SOLUTION", "OFFERING",
        "PROPOSITION", "VALUE", "BENEFIT", "ADVANTAGE", "FEATURE", "FUNCTION", "UTILITY", "USEFULNESS",
        "APPLICABILITY", "RELEVANCE", "SIGNIFICANCE", "IMPORTANCE", "IMPACT", "EFFECT", "INFLUENCE",
        "CONSEQUENCE", "OUTCOME", "RESULT", "ACHIEVEMENT", "SUCCESS", "ACCOMPLISHMENT", "ATTAINMENT",
        "REALIZATION", "FULFILLMENT", "SATISFACTION", "PLEASURE", "ENJOYMENT", "HAPPINESS", "JOY",
        "DELIGHT", "BLISS", "ECSTASY", "EUPHORIA", "CONTENTMENT", "COMFORT", "RELIEF", "EASE",
        "PEACE", "TRANQUILITY", "SERENITY", "CALM", "REST", "RELAXATION", "REPOSE", "LEISURE",
        "FREEDOM", "LIBERTY", "INDEPENDENCE", "AUTONOMY", "SELF-DETERMINATION", "CHOICE", "OPTION",
        "ALTERNATIVE", "POSSIBILITY", "OPPORTUNITY", "PROSPECT", "POTENTIAL", "CAPABILITY", "CAPACITY",
        "POWER", "STRENGTH", "FORCE", "ENERGY", "VIGOR", "VITALITY", "LIFE", "SPIRIT", "SOUL",
        "HEART", "MIND", "INTELLECT", "REASON", "LOGIC", "SENSE", "WISDOM", "KNOWLEDGE", "UNDERSTANDING",
        "INSIGHT", "AWARENESS", "CONSCIOUSNESS", "PERCEPTION", "INTUITION", "INSTINCT", "FEELING",
        "EMOTION", "PASSION", "DESIRE", "APPETITE", "HUNGER", "THIRST", "CRAVING", "URGE", "IMPULSE",
        "DRIVE", "MOTIVATION", "AMBITION", "ASPIRATION", "GOAL", "OBJECTIVE", "TARGET", "AIM",
        "PURPOSE", "INTENTION", "PLAN", "DESIGN", "SCHEME", "STRATEGY", "TACTIC", "METHOD", "APPROACH",
        "TECHNIQUE", "PROCEDURE", "PROCESS", "OPERATION", "ACTION", "ACTIVITY", "PRACTICE", "EXERCISE",
        "EFFORT", "ENDEAVOR", "UNDERTAKING", "PROJECT", "VENTURE", "ENTERPRISE", "BUSINESS", "COMPANY",
        "ORGANIZATION", "INSTITUTION", "AGENCY", "AUTHORITY", "ADMINISTRATION", "GOVERNMENT", "STATE",
        "NATION", "COUNTRY", "LAND", "REGION", "TERRITORY", "DOMAIN", "REALM", "WORLD", "UNIVERSE",
        "COSMOS", "NATURE", "ENVIRONMENT", "EARTH", "PLANET", "GLOBE", "WORLD", "UNIVERSE", "COSMOS",
        "SPACE", "GALAXY", "STAR", "SUN", "MOON", "PLANET", "COMET", "ASTEROID", "METEOR", "SATELLITE",
        "ORBIT", "GRAVITY", "FORCE", "ENERGY", "MATTER", "PARTICLE", "ATOM", "MOLECULE", "COMPOUND",
        "ELEMENT", "MINERAL", "ROCK", "STONE", "CRYSTAL", "GEM", "METAL", "ALLOY", "ORE", "SOIL",
        "SAND", "CLAY", "MUD", "DIRT", "DUST", "ASH", "SMOKE", "STEAM", "VAPOR", "MIST", "FOG",
        "CLOUD", "RAIN", "SNOW", "ICE", "HAIL", "SLEET", "DEW", "FROST", "GLACIER", "AVALANCHE",
        "LANDSLIDE", "EARTHQUAKE", "VOLCANO", "ERUPTION", "TSUNAMI", "FLOOD", "STORM", "HURRICANE",
        "TYPHOON", "CYCLONE", "TORNADO", "TWISTER", "WHIRLWIND", "BLIZZARD", "SNOWSTORM", "THUNDERSTORM",
        "LIGHTNING", "THUNDER", "RAINBOW", "AURORA", "ECLIPSE", "SOLSTICE", "EQUINOX", "SEASON",
        "SPRING", "SUMMER", "AUTUMN", "WINTER", "CLIMATE", "WEATHER", "TEMPERATURE", "HEAT", "COLD",
        "WARMTH", "COOLNESS", "HUMIDITY", "DRYNESS", "WIND", "BREEZE", "GUST", "BLAST", "STORM",
        "GALE", "HURRICANE", "TYPHOON", "CYCLONE", "TORNADO", "TWISTER", "WHIRLWIND", "BLIZZARD",
        "SNOWSTORM", "THUNDERSTORM", "LIGHTNING", "THUNDER", "RAINBOW", "AURORA", "ECLIPSE", "SOLSTICE",
        "EQUINOX", "SEASON", "SPRING", "SUMMER", "AUTUMN", "WINTER", "CLIMATE", "WEATHER", "TEMPERATURE",
        "HEAT", "COLD", "WARMTH", "COOLNESS", "HUMIDITY", "DRYNESS", "WIND", "BREEZE", "GUST",
        "BLAST", "STORM", "GALE", "FIRE", "FLAME", "BLAZE", "INFERNO", "CONFLAGRATION", "EXPLOSION",
        "DETONATION", "BLAST", "BOOM", "BANG", "CRASH", "COLLISION", "IMPACT", "HIT", "STRIKE",
        "BLOW", "PUNCH", "KICK", "SLAP", "SMACK", "BASH", "BANG", "THUMP", "THUD", "CLAP", "SLAM",
        "SMASH", "CRUSH", "SQUASH", "STOMP", "TRAMPLE", "TREAD", "WALK", "RUN", "JUMP", "LEAP",
        "HOP", "SKIP", "BOUNCE", "FLY", "SOAR", "GLIDE", "FLOAT", "DRIFT", "SAIL", "SWIM", "DIVE",
        "PLUNGE", "SINK", "SUBMERGE", "EMERGE", "SURFACE", "RISE", "ASCEND", "CLIMB", "SCALE",
        "MOUNT", "DESCEND", "FALL", "DROP", "PLUMMET", "TUMBLE", "ROLL", "SLIDE", "SLIP", "SKID",
        "GLIDE", "SWERVE", "VEER", "TURN", "TWIST", "BEND", "FLEX", "STRETCH", "REACH", "GRASP",
        "GRAB", "HOLD", "CLUTCH", "CLING", "HUG", "EMBRACE", "KISS", "TOUCH", "FEEL"};
const std::vector<std::string> ItemIdGenerator::subcategories = {
        "USAGE", "TEMP", "ERROR", "LOAD", "CAPACITY", "THROUGHPUT", "LATENCY", "BANDWIDTH",
        "EFFICIENCY", "DENSITY", "VOLATILITY", "DURATION", "FREQUENCY", "INTENSITY", "MAGNITUDE",
        "VARIANCE", "DEVIATION", "PRECISION", "ACCURACY", "RELIABILITY", "AVAILABILITY", "SUSTAINABILITY",
        "PERFORMANCE", "SPEED", "VELOCITY", "ACCELERATION", "FORCE", "POWER", "ENERGY", "WORK",
        "PRESSURE", "VOLUME", "DENSITY", "MASS", "WEIGHT", "GRAVITY", "TEMPERATURE", "HEAT",
        "LIGHT", "COLOR", "BRIGHTNESS", "LUMINANCE", "ILLUMINATION", "SOUND", "VOLUME", "PITCH",
        "TONE", "FREQUENCY", "RHYTHM", "MELODY", "HARMONY", "NOISE", "SIGNAL", "WAVE", "VIBRATION",
        "PULSE", "FLUX", "FLOW", "CURRENT", "STREAM", "WIND", "WAVE", "TIDE", "MOISTURE", "HUMIDITY",
        "DRYNESS", "WETNESS", "LIQUID", "SOLID", "GAS", "PLASMA", "CHEMISTRY", "COMPOSITION", "MIXTURE",
        "SOLUTION", "REACTION", "OXIDATION", "REDUCTION", "PH", "CONCENTRATION", "PURITY", "TOXICITY",
        "POLLUTION", "CONTAMINATION", "RADIATION", "IONIZATION", "CONDUCTIVITY", "RESISTIVITY", "CAPACITANCE",
        "INDUCTANCE", "MAGNETISM", "ELECTROMAGNETISM", "CHARGE", "VOLTAGE", "CURRENT", "RESISTANCE",
        "IMPEDANCE", "ADMITTANCE", "REACTANCE", "POLARIZATION", "OPTICS", "REFLECTION", "REFRACTION",
        "DIFFRACTION", "INTERFERENCE", "ABSORPTION", "TRANSMISSION", "EMISSION", "LUMINESCENCE",
        "FLUORESCENCE", "PHOSPHORESCENCE", "LASER", "FIBER", "OPTICS", "IMAGE", "PICTURE", "PHOTO",
        "GRAPHIC", "DESIGN", "PATTERN", "TEXTURE", "SHAPE", "SIZE", "DIMENSION", "ANGLE", "POSITION",
        "ORIENTATION", "DIRECTION", "LOCATION", "DISTANCE", "RANGE", "DEPTH", "HEIGHT", "WIDTH",
        "LENGTH", "AREA", "VOLUME", "DENSITY", "MASS", "WEIGHT", "FORCE", "TORQUE", "MOMENTUM",
        "IMPULSE", "ENERGY", "POWER", "WORK", "HEAT", "TEMPERATURE", "PRESSURE", "VIBRATION", "SHOCK",
        "STRESS", "STRAIN", "FATIGUE", "FAILURE", "BREAKAGE", "CRACK", "LEAK", "WEAR", "CORROSION",
        "EROSION", "FRICTION", "LUBRICATION", "COOLING", "HEATING", "VENTILATION", "FILTRATION",
        "SEPARATION", "PURIFICATION", "STERILIZATION", "DISINFECTION", "CLEANING", "WASHING", "DRAINING",
        "DRYING", "COATING", "PAINTING", "PLATING", "POLISHING", "GRINDING", "CUTTING", "DRILLING",
        "MILLING", "TURNING", "WELDING", "SOLDERING", "BONDING", "FASTENING", "JOINING", "ASSEMBLY",
        "DISASSEMBLY", "INSTALLATION", "REMOVAL", "REPLACEMENT", "REPAIR", "MAINTENANCE", "SERVICE",
        "INSPECTION", "TESTING", "CALIBRATION", "ADJUSTMENT", "ALIGNMENT", "BALANCING", "TUNING",
        "PROGRAMMING", "CONFIGURATION", "SETUP", "INITIALIZATION", "STARTUP", "SHUTDOWN", "RESET",
        "ENABLE", "DISABLE", "ACTIVATE", "DEACTIVATE", "OPEN", "CLOSE", "LOCK", "UNLOCK", "SWITCH",
        "SELECT", "CHANGE", "CONVERT", "TRANSFORM", "TRANSLATE", "ROTATE", "SCALE", "MIRROR",
        "COPY", "PASTE", "CUT", "DELETE", "INSERT", "REMOVE", "ADD", "SUBTRACT", "MULTIPLY", "DIVIDE",
        "INCREASE", "DECREASE", "EXPAND", "CONTRACT", "STRETCH", "COMPRESS", "INFLATE", "DEFLATE",
        "FILL", "EMPTY", "LOAD", "UNLOAD", "CHARGE", "DISCHARGE", "FUEL", "REFUEL", "PUMP", "SIPHON",
        "FLOW", "STREAM", "SPRAY", "SPREAD", "DISPERSE", "DISSIPATE", "EVAPORATE", "CONDENSE", "FREEZE",
        "MELT", "SOLIDIFY", "LIQUEFY", "GASIFY", "CRYSTALLIZE", "PRECIPITATE", "DEPOSIT", "SEDIMENT",
        "COAGULATE", "FLOCCULATE", "EMULSIFY", "HOMOGENIZE", "AGITATE", "STIR", "SHAKE", "MIX",
        "BLEND", "COMBINE", "SEPARATE", "SORT", "FILTER", "SIEVE", "SCREEN", "DISTILL", "EXTRACT",
        "ABSORB", "ADSORB", "IONIZE", "ELECTRIFY", "MAGNETIZE", "POLARIZE", "CHARGE", "DISCHARGE",
        "ENERGIZE", "DE-ENERGIZE", "ILLUMINATE", "LIGHT", "BRIGHTEN", "DIM", "SHADE", "COLOR",
        "TINT", "TONE", "SHADE", "HUE", "SATURATE", "DESATURATE", "FADE", "BLEACH", "DYE", "PAINT",
        "COAT", "COVER", "WRAP", "ENCLOSE", "PROTECT", "GUARD", "SHIELD", "DEFEND", "SECURE", "SAFEGUARD",
        "PRESERVE", "CONSERVE", "SAVE", "RESCUE", "RECOVER", "RETRIEVE", "RECLAIM", "RECYCLE", "REUSE",
        "REDUCE", "ELIMINATE", "ERADICATE", "DESTROY", "DEMOLISH", "RUIN", "DAMAGE", "HARM", "INJURE",
        "HURT", "WOUND", "KILL", "SLAY", "ASSASSINATE", "EXECUTE", "ELIMINATE", "ERADICATE", "EXTINGUISH",
        "ANNIHILATE", "OBLITERATE", "DECIMATE", "RAVAGE", "DEVASTATE", "DESOLATE", "WASTE", "SQUANDER",
        "DEPLETE", "EXHAUST", "DRAIN", "EXPEND", "CONSUME", "USE", "UTILIZE", "EMPLOY", "OPERATE",
        "MANAGE", "CONTROL", "COMMAND", "DIRECT", "GUIDE", "LEAD", "GOVERN", "RULE", "ADMINISTER",
        "SUPERVISE", "OVERSEE", "INSPECT", "MONITOR", "WATCH", "OBSERVE", "SURVEY", "SCAN", "SEARCH",
        "EXPLORE", "INVESTIGATE", "EXAMINE", "STUDY", "RESEARCH", "ANALYZE", "ASSESS", "EVALUATE",
        "APPRAISE", "REVIEW", "AUDIT", "CHECK", "TEST", "PROVE", "DEMONSTRATE", "EXHIBIT", "DISPLAY",
        "SHOW", "PRESENT", "ILLUSTRATE", "DEPICT", "PORTRAY", "REPRESENT", "SIMULATE", "IMITATE",
        "MIMIC", "REPLICATE", "REPRODUCE", "COPY", "DUPLICATE", "CLONE", "EMULATE", "MATCH", "EQUAL",
        "RIVAL", "COMPETE", "OPPOSE", "RESIST", "DEFY", "CONFRONT", "CHALLENGE", "CONTEND", "DISPUTE",
        "ARGUE", "DEBATE", "DISCUSS", "TALK", "SPEND", "CONVERSE", "COMMUNICATE", "SHARE", "EXCHANGE",
        "TRANSMIT", "TRANSFER", "TRANSPORT", "CARRY", "MOVE", "SHIFT", "RELOCATE", "DISPLACE", "REMOVE",
        "DISPOSE", "DISCARD", "ABANDON", "FORSAKE", "NEGLECT", "IGNORE", "NEGLECT", "IGNORE", "NEGLECT"};









#endif //WAFFLE_TS_KEY_MASTER_H
