#include <sawyer/Map.h>

#include <boost/foreach.hpp>
#include <iostream>
#include <sawyer/Assert.h>
#include <string>

template<class Key, class Value>
std::ostream& operator<<(std::ostream &o, const Sawyer::Container::Map<Key, Value> &map) {
    typedef Sawyer::Container::Map<Key, Value> Map;
    o <<"{";
    BOOST_FOREACH (const typename Map::Node &node, map.nodes())
        o <<" [" <<node.key() <<"]=" <<node.value();
    o <<" }";
    return o;
}

template<class Map>
void default_ctor() {
    std::cout <<"default constructor:\n";
    Map map;
    std::cout <<"    " <<map <<"\n";
    ASSERT_require(map.isEmpty());
    ASSERT_require(map.size()==0);
}

template<class Map>
void insert_one() {
    std::cout <<"insert one item at a time:\n";

    Map map;
    map.insert("one", 1);

    map
        .insert("two", 2)
        .insert("three", 3)
        .insert("four", 4);

    std::cout <<"    initial insertion: " <<map <<"\n";
    ASSERT_require(map.size()==4);
    ASSERT_require(map["one"]==1);
    ASSERT_require(map["two"]==2);
    ASSERT_require(map["three"]==3);
    ASSERT_require(map["four"]==4);

    typename Map::Value &two = map.insertMaybe("two", 22); // not inserted
    std::cout <<"    not inserted [two]=22: " <<map <<"\n";
    ASSERT_require(map.size()==4);
    ASSERT_require(map["two"]==2);
    ASSERT_require(two==2);

    typename Map::Value &uno = map.insertMaybe("uno", 1);
    std::cout <<"    inserted [uno]=1: " <<map <<"\n";
    ASSERT_require(map.size()==5);
    ASSERT_require(map["uno"]==1);
    ASSERT_require(uno==1);
}

template<class Map>
void insert_other() {
    std::cout <<"insert values from another map:\n";

    typedef Sawyer::Container::Map<const char*, double> OtherMap;
    OtherMap other;
    other.insert("pi", 3.14).insert("tau", 6.28).insert("e", 2.72);
    Map map;
    map.insert("one", 1).insert("two", 2).insert("three", 3);
    std::cout <<"    initial value for other: " <<other <<"\n";
    std::cout <<"    initial value for map: " <<map <<"\n";

    map.insertMultiple(other.nodes());
    std::cout <<"    inserted other into map: " <<map <<"\n";
    ASSERT_require(map.size()==6);
    ASSERT_require(map["e"]==2);
    ASSERT_require(map["one"]==1);
    ASSERT_require(map["pi"]==3);
    ASSERT_require(map["tau"]==6);
    ASSERT_require(map["three"]==3);
    ASSERT_require(map["two"]==2);

    // Const version without copy constructor
    Map map2;
    map2.insert("one", 1).insert("two", 2).insert("three", 3);
    const OtherMap *other2 = &other;
    map2.insertMultiple(other2->nodes());
    std::cout <<"    inserted other into map2: " <<map2 <<"\n";
    ASSERT_require(map2.size()==6);
    ASSERT_require(map2["e"]==2);
    ASSERT_require(map2["one"]==1);
    ASSERT_require(map2["pi"]==3);
    ASSERT_require(map2["tau"]==6);
    ASSERT_require(map2["three"]==3);
    ASSERT_require(map2["two"]==2);
}

template<class Map>
void find() {
    std::cout <<"find values by key:\n";
    Map map;
    map.insert("eye", 1).insert("nose", 2).insert("mouth", 3).insert("neck", 4).insert("", 100);
    std::cout <<"    initial map: " <<map <<"\n";

    typename Map::NodeIterator iter;
    iter = map.find("mouth");
    ASSERT_require(iter!=map.nodes().end());
    ASSERT_require(iter->key()=="mouth");
    ASSERT_require(iter->value()==3);

    iter = map.find("");
    ASSERT_require(iter!=map.nodes().end());
    ASSERT_require(iter->key()=="");
    ASSERT_require(iter->value()==100);

    iter = map.find("forehead");
    ASSERT_require(iter==map.nodes().end());
}

template<class Map>
void test_existence() {
    std::cout <<"test whether nodes exist:\n";
    Map map;
    map.insert("eye", 1).insert("nose", 2).insert("mouth", 3).insert("neck", 4).insert("", 100);
    std::cout <<"    initial map: " <<map <<"\n";

    ASSERT_require(map.exists("eye"));
    ASSERT_require(map.exists("nose"));
    ASSERT_require(map.exists("mouth"));
    ASSERT_require(map.exists("neck"));
    ASSERT_require(map.exists(""));

    ASSERT_forbid(map.exists("shoulders"));
}

template<class Map>
void clear_all() {
    std::cout <<"remove all nodes:\n";
    Map map;
    map.insert("eye", 1).insert("nose", 2).insert("mouth", 3).insert("neck", 4).insert("", 100);
    std::cout <<"    initial map: " <<map <<"\n";

    map.clear();
    std::cout <<"    cleared map: " <<map <<"\n";
    ASSERT_require(map.isEmpty());
    ASSERT_require(map.size()==0);
    ASSERT_require(!map.exists("mouth"));
}

template<class Map>
void erase_one() {
    std::cout <<"erase one node at a time:\n";
    Map map;
    map.insert("eye", 1).insert("nose", 2).insert("mouth", 3).insert("neck", 4).insert("", 100);
    std::cout <<"    initial map: " <<map <<"\n";

    map.erase("eye");
    std::cout <<"    removed \"eye\": " <<map <<"\n";
    ASSERT_require(map.size()==4);
    ASSERT_forbid(map.exists("eye"));

    map.erase("");
    std::cout <<"    removed \"\": " <<map <<"\n";
    ASSERT_require(map.size()==3);
    ASSERT_forbid(map.exists(""));

    map
        .erase("neck")
        .erase("mouth");
    std::cout <<"    removed \"neck\" and \"mouth\": " <<map <<"\n";
    ASSERT_require(map.size()==1);
    ASSERT_forbid(map.exists("neck"));
    ASSERT_forbid(map.exists("mouth"));

    map.erase("shoulder");
    std::cout <<"    removed \"shoulder\" (not): " <<map <<"\n";
    ASSERT_require(map.size()==1);

    map.erase("nose");
    std::cout <<"    removed \"nose\": " <<map <<"\n";
    ASSERT_require(map.isEmpty());
}

template<class Map>
void erase_other() {
    std::cout <<"erase keys that are in another map:\n";
    
    Map map;
    map.insert("eye", 1).insert("nose", 2).insert("mouth", 3).insert("neck", 4).insert("", 100);
    std::cout <<"    initial map: " <<map <<"\n";

    typedef Sawyer::Container::Map<const char*, double> Other;
    Other other;
    other.insert("neck", 1.2).insert("forehead", 99.99).insert("eye", 1.1).insert("", 2.2);
    std::cout <<"    to remove: " <<other <<"\n";

    map.eraseMultiple(other.keys());
    std::cout <<"    result: " <<map <<"\n";
    ASSERT_require(map.size()==2);
    ASSERT_require(map["mouth"]==3);
    ASSERT_require(map["nose"]==2);
}

template<class Map>
void iterators() {
    std::cout <<"iterator functionality:\n";

    Map map;
    map.insert("eye", 1).insert("nose", 2).insert("mouth", 3).insert("neck", 4).insert("", 100);
    std::cout <<"    initial map: " <<map <<"\n";

    // Implicit conversions
    typename Map::NodeIterator ni = map.find("mouth");

    typename Map::ConstNodeIterator cni = ni;

    typename Map::ConstKeyIterator cki = ni;
    typename Map::ConstKeyIterator ck2 = cni;

    typename Map::ValueIterator vi = ni;

    typename Map::ConstValueIterator cvi = vi;
    typename Map::ConstValueIterator cvi2 = ni;
    typename Map::ConstValueIterator cvi3 = cni;

    // Node iterators
    ASSERT_require(ni!=map.nodes().end());

    ASSERT_require(ni->key()=="mouth");
    ASSERT_require((*ni).key()=="mouth");
    ASSERT_require(ni->value()==3);
    ASSERT_require((*ni).value()==3);

    ASSERT_require(ni!=map.find("eye"));
    ASSERT_forbid(ni==map.find("eye"));

    ASSERT_require(ni==ni);
    ASSERT_forbid(ni!=ni);

    // Const node iterators
    ASSERT_require(cni!=map.nodes().end());

    ASSERT_require(cni->key()=="mouth");
    ASSERT_require((*cni).key()=="mouth");
    ASSERT_require(cni->value()==3);
    ASSERT_require((*cni).value()==3);

    ASSERT_require(cni!=map.find("eye"));
    ASSERT_forbid(cni==map.find("eye"));

    ASSERT_require(cni==cni);
    ASSERT_forbid(cni!=cni);
    ASSERT_require(cni==ni);
    ASSERT_forbid(cni!=ni);

    // Const key iterators
    ASSERT_require(cki!=map.nodes().end());

    ASSERT_require(*cki=="mouth");
    ASSERT_require(cki->size()==5);

    ASSERT_require(cki!=map.find("eye"));
    ASSERT_forbid(cki==map.find("eye"));

    ASSERT_require(cki==cki);
    ASSERT_forbid(cki!=cki);
    ASSERT_require(cki==ni);
    ASSERT_forbid(cki!=ni);
    ASSERT_require(cki==cni);
    ASSERT_forbid(cki!=cni);

    // Value iterators
    ASSERT_require(vi!=map.nodes().end());

    ASSERT_require(*vi==3);

    ASSERT_require(vi!=map.find("eye"));
    ASSERT_forbid(vi==map.find("eye"));

    ASSERT_require(vi==vi);
    ASSERT_forbid(vi!=vi);
    ASSERT_require(vi==ni);
    ASSERT_forbid(vi!=ni);
    ASSERT_require(vi==cni);
    ASSERT_forbid(vi!=cni);
    ASSERT_require(vi==cki);
    ASSERT_forbid(vi!=cki);

    // Const value iterators
    ASSERT_require(cvi!=map.nodes().end());

    ASSERT_require(*cvi==3);

    ASSERT_require(cvi!=map.find("eye"));
    ASSERT_forbid(cvi==map.find("eye"));

    ASSERT_require(cvi==cvi);
    ASSERT_forbid(cvi!=cvi);
    ASSERT_require(cvi==ni);
    ASSERT_forbid(cvi!=ni);
    ASSERT_require(cvi==cni);
    ASSERT_forbid(cvi!=cni);
    ASSERT_require(cvi==cki);
    ASSERT_forbid(cvi!=cki);
    ASSERT_require(cvi==vi);
    ASSERT_forbid(cvi!=vi);

    // Modification via node iterator
    ni->value() = 33;
    ASSERT_require(ni->value()==33);
    ASSERT_require(cni->value()==33);
    ASSERT_require(*vi==33);
    ASSERT_require(*cvi==33);

    // Modification via value iterator
    *vi = 44;
    ASSERT_require(ni->value()==44);
    ASSERT_require(cni->value()==44);
    ASSERT_require(*vi==44);
    ASSERT_require(*cvi==44);

    // Node iterator increment
    typename Map::NodeIterator ni2 = ni++;
    ASSERT_require(ni->key()=="neck");
    ASSERT_require(ni2->key()=="mouth");
    typename Map::NodeIterator ni3 = ++ni;
    ASSERT_require(ni->key()=="nose");
    ASSERT_require(ni3->key()=="nose");
}

template<class Map>
void erase_iterator() {
    std::cout <<"erase nodes according to iterator:\n";
    std::cout <<"    NOT IMPLEMENTED YET\n";
}

    

int main() {
    typedef std::string Key;
    typedef int Value;
    typedef Sawyer::Container::Map<Key, Value> Map;

    default_ctor<Map>();
    insert_one<Map>();
    insert_other<Map>();
    find<Map>();
    test_existence<Map>();
    clear_all<Map>();
    erase_one<Map>();
    erase_other<Map>();
    iterators<Map>();
    erase_iterator<Map>();
}
