#ifndef GENERIC_FACTORY_HPP
#define GENERIC_FACTORY_HPP

#include <memory>
#include <functional>
#include <unordered_map>
#include <mutex>

template<typename Parent, typename... Args>
class GenericFactory {
	std::unordered_map<std::string, std::function<std::unique_ptr<Parent>(Args...)>> _children;
	std::mutex _mutex;

	GenericFactory() = default; // No need to forbid copying or moving, because it's impossible to obtain an instance from outside

	static GenericFactory &getGenericFactory()
	{
		static GenericFactory factory;
		return factory;
	}

public:

	/*!
	* \brief Registers a constructor of a possible child
	* \param The name of the child type
	* \param A function that returns a unique_ptr to a new constructed child when called, taking its address as argument
	* \return True if successfully added, false if already exists
	*
	* \note It's thread safe
	* \note In a usual case, you may use the REGISTER_CHILD_INTO_FACTORY(); macro
	*/
	static bool registerChild(const std::string &name, std::function<std::unique_ptr<Parent>(Args...)> maker)
	{
		auto &factory = getGenericFactory();
		std::lock_guard<std::mutex> guard(factory._mutex);
		auto found = factory._children.find(name);
		if(found != factory._children.end())
			return false;
		factory._children[name] = maker;
		return true;
	}

	/*!
	* \brief Registers a constructor of a child
	* The template argument is the child class being created
	* \param The name of the child
	* \return True if successfully added, false if already exists
	*
	* \note It's thread safe
	* \note In a usual case, you may use the REGISTER_CHILD_INTO_FACTORY(); macro
	*/
	template <typename Child>
	static bool registerChild(const std::string &name)
	{
		return registerChild(name, [](Args... args) {
			return std::make_unique<Child>(args...);
		});
	}

	/*!
	* \brief Unregisters a constructor of a child
	* \param The name of the child
	* \return True if it was registered, false if it wasn't
	*
	* \note It's thread safe
	*/
	static bool unregisterChild(const std::string &name)
	{
		auto &factory = getGenericFactory();
		std::lock_guard<std::mutex> guard(factory._mutex);
		auto found = factory._children.find(name);
		if(found == factory._children.end())
			return false;
		factory._children.erase(found);
		return true;
	}

	/*!
	* \brief Creates a child of the given name with the following arguments fed to its constructor
	* \param The name of the child
	* \param Constructor arguments (as many as necessary)
	*
	* \note It's thread safe, long construction time will delay the construction of others
	*/
	static std::unique_ptr<Parent> createChild(const std::string &name, Args... args)
	{
		auto &factory = getGenericFactory();
		std::lock_guard<std::mutex> guard(factory._mutex);
		auto found = factory._children.find(name);
		if(found == factory._children.end())
			throw(std::runtime_error("Unknown child: " + name));
		return found->second(args...);
	}
};

namespace GenericFactoryInternals {
template<typename Returned, typename Downcast, typename Used, typename... Args>
std::unique_ptr<Returned> createFunction(std::shared_ptr<Used> primary, Args... args) {
	return std::make_unique<Returned>(std::dynamic_pointer_cast<Downcast>(primary), args...);
}

template<typename Returned, typename Downcast, typename Used, typename... Args>
std::unique_ptr<Returned> createFunction(std::unique_ptr<Used> primary, Args... args) {
	return std::make_unique<Returned>(std::unique_ptr<Downcast>(dynamic_cast<Downcast*>(primary.release())), args...);
}

template<typename Returned, typename Downcast, typename Used, typename... Args>
std::unique_ptr<Returned> createFunction(Used* primary, Args... args) {
	return std::make_unique<Returned>(dynamic_cast<Downcast*>(primary), args...);
}

struct IfYouSeeThisTypeInErrorMessageThenYouNeedToUseADifferentPointerType {};

template<typename, typename, typename, typename, typename...>
struct AcceptedPointerTypeHelper {
//	using type = IfYouSeeThisTypeInErrorMessageThenYouNeedToUseADifferentPointerType*;
};

template<typename Constructed, typename FromParent, typename FromChild, typename... Args>
struct AcceptedPointerTypeHelper<Constructed, FromParent, FromChild, typename std::enable_if<std::is_constructible<Constructed, std::shared_ptr<FromChild>, Args...>::value, FromChild>::type, Args...> {
	using type = std::shared_ptr<FromParent>;
};

template<typename Constructed, typename FromParent, typename FromChild, typename... Args>
struct AcceptedPointerTypeHelper<Constructed, FromParent, FromChild, typename std::enable_if<std::is_constructible<Constructed, std::unique_ptr<FromChild>, Args...>::value
		&& !std::is_constructible<Constructed, std::shared_ptr<FromChild>, Args...>::value, FromChild>::type, Args...> {
	using type = std::unique_ptr<FromParent>;
};

template<typename Constructed, typename FromParent, typename FromChild, typename... Args>
struct AcceptedPointerTypeHelper<Constructed, FromParent, FromChild, typename std::enable_if<std::is_constructible<Constructed, FromChild*, Args...>::value, FromChild>::type, Args...> {
	using type = FromParent*;
};

template<typename Constructed, typename FromParent, typename FromChild, typename... Args>
using AcceptedPointerType = typename AcceptedPointerTypeHelper<Constructed, FromParent, FromChild, FromChild, Args...>::type;
}

template<typename ConstructedParent, typename PrimaryParent, typename... Args>
class GenericSecondaryFactory {
	static_assert(std::is_polymorphic<std::decay_t<decltype(*std::declval<PrimaryParent>())>>::value,
				  "Class choosing the right descendant in GenericSecondaryFactory must be a pointer to a polymorphic class");

	std::unordered_map<size_t, std::function<std::unique_ptr<ConstructedParent>(PrimaryParent, Args...)>> _children;
	std::mutex _mutex;

	GenericSecondaryFactory() = default;

	static GenericSecondaryFactory &getGenericSecondaryFactory()
	{
		static GenericSecondaryFactory factory;
		return factory;
	}

public:
	/*!
	* \brief Registers a constructor of a child
	* The template argument is specific type the object must be of
	* \param A function that returns a unique_ptr to a new constructed child when called, taking its address as argument
	* \return True if successfully added, false if already exists
	*
	* \note It's thread safe
	* \note In a usual case, you may use the REGISTER_SECONDARY_CHILD_INTO_FACTORY(); macro
	*/
	template <typename PrimaryChild>
	static bool registerChild(std::function<std::unique_ptr<ConstructedParent>(PrimaryParent, Args...)> maker)
	{
		auto &factory = getGenericSecondaryFactory();
		std::lock_guard<std::mutex> guard(factory._mutex);
		size_t sought = typeid(PrimaryChild).hash_code();
		auto found = factory._children.find(sought);
		if(found != factory._children.end())
			return false;
		factory._children[sought] = maker;
		return true;
	}

	/*!
	* \brief Registers a constructor of a child
	* The template arguments are the type being created and the specific type the object must be of, in order
	* \return True if successfully added, false if already exists
	*
	* \note It's thread safe
	* \note In a usual case, you may use the REGISTER_SECONDARY_CHILD_INTO_FACTORY(); macro
	*/
	template <typename ConstructedChild, typename PrimaryChild>
	static bool registerChild()
	{
		return registerChild<PrimaryChild>([](PrimaryParent primary, Args... args) -> std::unique_ptr<ConstructedParent> {
			return GenericFactoryInternals::createFunction<ConstructedChild, PrimaryChild>(primary, args...);
		});
	}

	/*!
	* \brief Unregisters a constructor of a child
	* The template argument is specific type the object must be of
	* \return True if it was registered, false if it wasn't
	*
	* \note It's thread safe
	*/
	template <typename PrimaryChild>
	static bool unregisterChild()
	{
		auto &factory = getGenericSecondaryFactory();
		std::lock_guard<std::mutex> guard(factory._mutex);
		auto found = factory._children.find(typeid(PrimaryChild).hash_code());
		if(found == factory._children.end())
			return false;
		factory._children.erase(found);
		return true;
	}

	/*!
	* \brief Creates a child tied with the class returned by calling * on the given argument
	* \param The class to decide the returned type
	* \param Constructor arguments (as many as necessary)
	*
	* \note It's thread safe, long construction time will delay the construction of others
	*/
	static std::unique_ptr<ConstructedParent> createChild(PrimaryParent primary, Args... args)
	{
		static_assert(std::is_base_of< std::decay_t<decltype(*std::declval<PrimaryParent>())>, std::decay_t<decltype(*primary)>>::value,
					  "GenericSecondaryFactory::createChild needs a pointer to a class derived from the set parent");
		auto &factory = getGenericSecondaryFactory();
		std::lock_guard<std::mutex> guard(factory._mutex);
		auto found = factory._children.find(typeid(*primary).hash_code());
		if(found == factory._children.end())
			throw(std::runtime_error("Unknown child related to class: " + std::string(typeid(*primary).name())));
		return found->second(primary, args...);
	}
};

/*!
* \brief Macro to hide the ugly but convenient parts when registering children, if the child's name is Dummy, class is ChildDummy, it's returned as an IChild
* and takes float and int as arguments, use:
* REGISTER_CHILD_INTO_FACTORY(IChild, ChildDummy, "Dummy", float, int)
* \note CANNOT be used in headers, must be in a source file, otherwise it will produce obscure linker errors
*/
#define REGISTER_CHILD_INTO_FACTORY(INTERFACE_TYPENAME, CHILD_TYPENAME, CHILD_NAME, ...) \
namespace GenericFactoryInternals { \
const bool INTERFACE_TYPENAME##_Registered = GenericFactory<INTERFACE_TYPENAME, ##__VA_ARGS__>::registerChild<CHILD_TYPENAME>(CHILD_NAME); \
} \

/*!
* \brief Macro to hide the ugly but convenient parts when registering secondary children, if the child's class is Dummy, it is constructed when the class is DummyGUI,
*  it's returned as an ISecondaryChild and takes Dummy, float and int as arguments, use:
* REGISTER_SECONDARY_CHILD_INTO_FACTORY(ISecondaryChild, IChild, DummyGUI, Dummy, float, int)
* \note CANNOT be used in headers, must be in a source file, otherwise it will produce obscure linker errors
*/
#define REGISTER_SECONDARY_CHILD_INTO_FACTORY(CONSTRUCTED_INTERFACE_TYPENAME, PRIMARY_INTERFACE_TYPENAME, CONSTRUCTED_CHILD_TYPENAME, PRIMARY_CHILD_TYPENAME, ...) \
namespace GenericFactoryInternals { \
const bool CONSTRUCTED_INTERFACE_TYPENAME##_From##PRIMARY_INTERFACE_TYPENAME##_Registered = \
GenericSecondaryFactory<CONSTRUCTED_INTERFACE_TYPENAME, AcceptedPointerType<CONSTRUCTED_CHILD_TYPENAME, PRIMARY_INTERFACE_TYPENAME, PRIMARY_CHILD_TYPENAME, ##__VA_ARGS__>, ##__VA_ARGS__>::registerChild<CONSTRUCTED_CHILD_TYPENAME, PRIMARY_CHILD_TYPENAME>(); \
} \

#endif // GENERIC_FACTORY_HPP
