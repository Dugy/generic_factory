# Generic Factory

Factories are a very useful pattern in object oriented programming. They however require including and indexing all these classes at one place, requiring modification when a new creatable subclass is added. This library allows to avoid this annoyance.

## Usage

Because the classes aren’t included at all, they don’t need headers at all:

```C++
class TextWidget : public Widget {
	std::string _text;
	float _fontSize;
public:
	TextWidget(const nlohmann::json& source) {
		fromJson(source);
	}
	void fromJson(const nlohmann::json& source) override {
		_text = source["text"];
		_fontSize = source["size"];
	}
	void draw(DrawingContext& context) override {
		context->drawText(_text, _fontSize);
	}
};
REGISTER_CHILD_INTO_FACTORY(Widget, TextWidget, "Text", const nlohmann::json&);
```

The factory can be used for example like this:

```C++
std::unique_ptr<Widget> widget =
		GenericFactory<Widget, const nlohmann::json&>
		::createChild(it.key(), it.value());
```

If there is some specific class that uses a specific subclass of the created class, it has to be done somewhat differently.

First, the used subclass needs a header, because it’s used by the other class (unless they are in the same file). The rest assumes that class `TextView` has this kind of access to class `TextWidget` and is its `friend`. The definition of class `TextView`:

```C++
class TextView : public WidgetView {
	TextWidget* _widget;
	std::vector<uint8_t> _serialised;
public:
	TextView(TextWidget* widget) : _widget(widget) {
	}
	const std::string& serialise() override {
		if (_serialised.empty()) {
			_serialised = toBase64(_widget._text);
			_serialised.push_back(':');
			_serialised.append(std::to_string(_widget._fontSize));
		}
		return _serialised;
	}
};
REGISTER_SECONDARY_CHILD_INTO_FACTORY(WidgetView, Widget, TextView, WidgetView);
```

Now, instances of any subclasses can be created this way:

```C++
std::unique_ptr widget =
		GenericFactory<Widget, const nlohmann::json&>
		::createChild(it.key(), it.value());
std::unique_ptr<WidgetView> view =
		GenericSecondaryFactory<WidgetView, Widget*>
		::createChild(widget.get());
```

Note that this is just an example, I am not making any GUI system and I have never written the other functions. A working code used for testing is part of this Github repository.

## The idea

The factory usually needs to be defined in its own source and header. Also, adding new classes requires remembering they have to be added into the factory as well (because it doesn’t follow the single responsibility principle by acting as some sort of virtual constructor of the common parent class).

It would be ideal if the specific classes were not included anywhere and the factory didn’t need to be declared at all. And it can be done.

The tools to define classes that can work with any possible class are templates. This can be used to create objects this way:

```C++
std::unique_ptr<Widget> widget =
		GenericFactory<Widget>::createChild(name);
```

Invoking the factory was the easy part. Now, how to make specific subclasses make the factory take them into consideration?

The subclasses are defined in different compilation units and making them available at the location of factory invocation would defeat the purpose. This can be dealt with using the singleton pattern, a way to have a global instance of an object, accessible from anywhere (it has to be properly encapsulated to prevent its abuse).

Singletons can be templated to have an instance for every set of template parameters.

The singleton is a way to share data between unrelated compilation units. But it’s done only at runtime, individual subclasses have to register themselves into the singleton during the program’s initialisation. A registration function each of them implements would again require some location with all these classes included and listed, defeating the purpose again.

Now, is there a way to execute code without it being called from main or from anything whose call stack begins with main? Well, actually, there is one. Initialisation of global variables. It has horrendous potential for abuse, so it has to be used with great care.

Global variables are initialised before main is called and can be set to the return value of a function (if it’s a dynamically loaded library added to the program later, it’s done when they are loaded). A function that registers the class into the singleton. An inconvenience of this approach is that it’s not possible to create more groups of creatable objects, but it’s unlikely to be ever needed. So each subclass needs this call in its source file (cannot be in a header, because the global variable would be defined once for every location it’s included in):

```C++
namespace NothingToSeeHere {
const bool definedTextWidget = GenericFactory<Widget>::
		registerChild<TextWidget>("Text");
}
```

The global variable itself serves no purpose, improperly changing it or accessing it cannot do any damage.

This code is really confusing, so it is better wrapped in a macro:

```C++
REGISTER_CHILD_INTO_FACTORY(Widget, TextWidget, "Text");
```
