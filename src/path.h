// -*- c++ -*-

class PathNode;

// @OBJDEF NodeLink
// sptr().visit("(%s).n"%obj, n)
// sptr().clear("(%s).n"%obj, n)
struct NodeLink {
    NodeLink(double d, PathNode* n);
    double d;
    sptr<PathNode> n;
};
static inline const bool operator<(const NodeLink& a, const NodeLink& b) {
    return a.d>b.d;
}

class PathNode : public BaseActor {
public: DECLARE_OBJECT;
    typedef pair<double, sptr<PathNode> > GoalPair;
    typedef map<sptr<PathNode>, GoalPair> GoalCache;

    PathNode();
    bool clips(Actor* o) {
	return false;
    }

    ~PathNode() {
    }
    CLASSN(PathNode);
    void init();
    //bool can_see_from(vect3d v, vector<Area*>& areas);
    inline bool can_see_from(vect3d pt, Area* area, bool flooronly = false);
    inline bool can_see_from(Area* area);
    inline bool can_see_from(vect3d v);
    void readargs(DataInput& ts);
    GoalPair getnextnode(PathNode* goal);

    void fillpathto(PathNode* goal);

    bool debug_onpath;
private:
    sptr<PathNode> tgoal;
    sptr<PathNode> nextn;
    double d2g;

    vector<NodeLink> onodes;

    set<int> zone;
    //set<int>* tzone;
    map< sptr<PathNode>, pair<double, sptr<PathNode> > > gcache;
    friend class NodeAppearance;
};

inline NodeLink::NodeLink(double d, PathNode* n) : d(d),n(n) { }
