#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <memory>
#include <set>
#include <sstream>
#include <system_error>
#ifdef HAVE_READLINE
# include <readline/readline.h>
# include <readline/history.h>
#endif
#ifdef _WIN32
# include <Windows.h>
# include <io.h>      // _setmode, _fileno
# include <fcntl.h>   // _O_U8TEXT
#endif
#ifdef HAVE_BOOST_LOCALE
# include <locale>
# include <boost/locale.hpp>
# include <boost/locale/collator.hpp>
# ifdef _WIN32
#  pragma comment(lib, "libboost_system-vc140-mt-s-1_61.lib") // вот же ж блин...
# endif
#endif
#if HAVE_BOOST_FILESYSTEM
# include <boost/filesystem.hpp>
# include <boost/filesystem/fstream.hpp>
# include <codecvt>
#endif

/*
 * Наконец, удалось побидить виндузную консоль. Теперь не нужно перекодировать
 * исходник, чтобы собрать работающую версию под windows. Теперь консоль
 * переключается в юникодный режим, а все строки внутри в UTF-8.
 *
 * Кроме того, я прикрутил-таки boost для реализации регистронезависимых
 * команд.
 *
 * Куда можно двигаться:
 *  - растащить исходник на несколько файлов .h и .cxx
 *  - написать документацию к классам
 *  - сделать алиасы командам
 *  - переделать токенизатор команды (boost)
 *  - сделать загрузку всех файлов армий из каталога (boost filesystem?)
 *
 */

bool ieq(const std::string& s1, const std::string& s2)
{
#ifdef HAVE_BOOST_LOCALE
	using namespace std;
	using namespace boost::locale;
	static generator gen;
	static locale loc = gen("ru_RU.UTF-8");
	int res = use_facet<collator<char> >(loc).compare(collator_base::primary, s1, s2);
	return res == 0;
#else
	return s1 == s2;
#endif
}

class UserInterface
{
private:
	UserInterface()
		: m_good(true)
	{
#ifdef _WIN32
		// Save old codepage and set new code page to UTF-8
		m_oldOutputCodePage = ::GetConsoleOutputCP();
		::SetConsoleOutputCP(65001);

		// different hack for input
		m_inputHandle = ::GetStdHandle( STD_INPUT_HANDLE );
		DWORD consoleMode;
		m_inputIsFromConsole = !! ::GetConsoleMode( m_inputHandle, &consoleMode );
		_setmode( _fileno( stdin ), _O_U8TEXT );
	}
	~UserInterface()
	{
		::SetConsoleOutputCP(m_oldOutputCodePage);
#endif /* _WIN32 */
	}
public:
	static UserInterface& instance()
	{
		static UserInterface self;
		return self;
	}
	bool good() const
	{
		return m_good;
	}
	void prompt(const char* prompt)
	{
		m_prompt = prompt;
	}
	std::string get_command()
	{
		std::string r;
#ifdef HAVE_READLINE
		char* p = readline(m_prompt.c_str());
		if(p)
		{
			r = p;
			free(p);
		}
		else
			m_good = false;
#elif defined _WIN32
		wchar_t buf[1024];
		char buf2[2048];
		int len = -1;
		*this << m_prompt;
		if(m_inputIsFromConsole)
		{
			DWORD nCharactersRead = 0;
			bool const readSucceeded = !! ::ReadConsoleW(m_inputHandle, buf, static_cast< DWORD >( sizeof(buf)/sizeof(buf[0]) ), &nCharactersRead, nullptr);
			if( ! readSucceeded )
			{
				m_good = false;
				return r;
			}
			wchar_t* p = wcschr(buf, L'\r');
			if( ! p )
			{
				m_good = false;
				return r;
			}
			len = p - buf;
		}
		else
		{
			std::wcin.getline(buf, sizeof(buf));
			m_good = std::wcin.good();
		}
		int len2 = ::WideCharToMultiByte(CP_UTF8, 0, buf, len, buf2, sizeof(buf2), NULL, NULL);
		return std::string(buf2, len2);
#else
		*this << m_prompt;
		std::getline(std::cin, r);
		m_good = std::cin.good();
#endif
		return r;
	}
	void unit_report(const std::string& id, const std::string& text)
	{
		*this << id << ": " << text << std::endl;
	}
	template<typename T>
	UserInterface& operator<<(const T& x)
	{
#ifdef _WIN32
		std::ostringstream ss;
		ss << x;
		printf("%s", ss.str().c_str());
#else
		std::cout << x;
#endif
		return *this;
	}
	// need non-template overload to accept std::endl
	typedef std::ostream& (*ostream_manipulator)(std::ostream&);
	UserInterface& operator<<(ostream_manipulator pf)
	{
		return operator<< <ostream_manipulator> (pf);
	}
private:
	bool m_good;
	std::string m_prompt;
#ifdef _WIN32
	UINT m_oldOutputCodePage;
	HANDLE m_inputHandle;
	bool m_inputIsFromConsole;
#endif
};

template<class T>
struct StringMatchHelper
{
	bool (T::*method)();
	const char* cmd;
	inline bool matches(const std::string& s) const
	{
		return ieq(s, cmd);
	}
	inline bool execute(T* obj) const
	{
		return (obj->*method)();
	}
};

class Unit
{
public:
	Unit(const std::string& name, const std::string& type, const std::string& army)
		: m_name(name)
		, m_type(type)
		, m_army(army)
	{
	}
	virtual ~Unit()
	{
	}
	std::string name() const
	{
		return m_name;
	}
	std::string type() const
	{
		return m_type;
	}
	std::string army() const
	{
		return m_army;
	}
	virtual bool command(const std::string& cmd)
	{
		static const StringMatchHelper<Unit> cmds[] = {
			{ &Unit::ping, "ping" },
		};
		for(size_t i = 0; i < sizeof(cmds)/sizeof(cmds[0]); ++i)
		{
			if(cmds[i].matches(cmd))
				return cmds[i].execute(this);
		}
		return false;
	}
	void report(const std::string& phrase)
	{
		UserInterface::instance().unit_report(name(), phrase);
	}
	virtual void dump(std::ostream& s) const
	{
		s << "Unit: " << m_name << " (type: " << m_type << ", army: " << m_army << ")";
	}
private:
	bool ping()
	{
		report(m_type + " ещё в строю");
		return true;
	}
private:
	std::string m_name;
	std::string m_type;
	std::string m_army;
};

class WalkingUnit: public Unit
{
	typedef Unit baseClass;
public:
	WalkingUnit(const std::string& name, const std::string& type, const std::string& army)
		: baseClass(name, type, army)
	{
	}
	virtual bool command(const std::string& cmd)
	{
		static const StringMatchHelper<WalkingUnit> cmds[] = {
			{ &WalkingUnit::move_left, "влево" },
			{ &WalkingUnit::move_right, "вправо" },
			{ &WalkingUnit::move_forward, "вперёд" },
			{ &WalkingUnit::move_backward, "назад" },
		};
		for(size_t i = 0; i < sizeof(cmds)/sizeof(cmds[0]); ++i)
		{
			if(cmds[i].matches(cmd))
				return cmds[i].execute(this);
		}
		return baseClass::command(cmd);
	}
	virtual void dump(std::ostream& s) const override
	{
		s << "Walking";
		baseClass::dump(s);
	}
protected:
	bool move_left()
	{
		report("Moving left");
		return true;
	}
	bool move_right()
	{
		report("Moving right");
		return true;
	}
	bool move_forward()
	{
		report("Moving forward");
		return true;
	}
	bool move_backward()
	{
		report("Moving backward");
		return true;
	}
};

class FlyingUnit: public WalkingUnit // so it can move horizontally
{
	typedef WalkingUnit baseClass;
public:
	FlyingUnit(const std::string& name, const std::string& type, const std::string& army)
		: baseClass(name, type, army)
	{
	}
	virtual bool command(const std::string& cmd) override
	{
		static const StringMatchHelper<FlyingUnit> cmds[] = {
			{ &FlyingUnit::move_up, "вверх" },
			{ &FlyingUnit::move_down, "вниз" },
		};
		for(size_t i = 0; i < sizeof(cmds)/sizeof(cmds[0]); ++i)
		{
			if(cmds[i].matches(cmd))
				return cmds[i].execute(this);
		}
		return baseClass::command(cmd);
	}
	virtual void dump(std::ostream& s) const override
	{
		s << "Flying";
		baseClass::dump(s);
	}
protected:
	bool move_up()
	{
		report("Moving up");
		return true;
	}
	bool move_down()
	{
		report("Moving down");
		return true;
	}
private:
};

class UnitFactory
{
private:
	UnitFactory()
	{
	}
	UnitFactory& operator = (const UnitFactory&);
public:
	static UnitFactory& instance()
	{
		static UnitFactory self;
		return self;
	}
	Unit* make(const std::string& name, const std::string& type, const std::string& army)
	{
#define MK(nm, cls) if(ieq(type, nm)) return new cls(name, type, army);
		MK("пехотинец", WalkingUnit);
		MK("танк", WalkingUnit);
		MK("вертолёт", FlyingUnit);
#undef MK
		throw std::runtime_error("Unknown unit type");
	}
};

class Tokenizer
{
public:
	Tokenizer(const std::string& s)
		: m_ss(s)
	{
		m_ss.peek();
	}
	bool has() const
	{
		return m_ss.good();
	}
	std::string get()
	{
		std::string r;
		m_ss >> r;
		return r;
	}
	std::vector< std::string > all()
	{
		std::vector< std::string > r;
		while(has())
			r.push_back(get());
		return std::move(r);
	}
private:
	std::istringstream m_ss;
};

class Command
{
public:
	bool parse(const std::string& s)
	{
		std::vector< std::string > cmd = Tokenizer(s).all();
		if(ieq(cmd[0], "и") && cmd.size() == 2)
		{
			m_cmd = cmd[1];
			return true;
		}
		else
			m_name = m_type = m_army = m_cmd = "";
		if(cmd.size() == 2)
		{
			if(! (ieq(cmd[0], "все") || ieq(cmd[0], "all")))
				m_name = cmd[0];
			m_cmd = cmd[1];
			return true;
		}
		if(ieq(cmd[0], "армия") && cmd.size() == 3)
		{
			m_army = cmd[1];
			m_cmd = cmd[2];
			return true;
		}
		if(ieq(cmd[0], "каждый") && cmd.size() == 3)
		{
			m_type = cmd[1];
			m_cmd = cmd[2];
			return true;
		}
		return false;
	}
	bool matches(const Unit& u) const
	{
		if(! m_name.empty() && m_name != u.name())
			return false;
		if(! m_type.empty() && m_type != u.type())
			return false;
		if(! m_army.empty() && m_army != u.army())
			return false;
		return true;
	}
	operator std::string() const
	{
		return m_cmd;
	}
private:
	std::string m_name;
	std::string m_type;
	std::string m_army;
	std::string m_cmd;
};

class Game
{
	friend class Command;
public:
	Game()
	{
	}
	~Game()
	{
	}
#if HAVE_BOOST_FILESYSTEM
	void load(const boost::filesystem::path& p)
	{
		boost::filesystem::ifstream f(p);
		if(! f.is_open())
			throw std::system_error(errno, std::system_category(), p.string().c_str());
		std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		load(f, conv.to_bytes(p.stem().wstring()));
	}
#else
	void load(const std::string& filename) /* No unicode support */
	{
		std::ifstream f(filename);
		if(! f.is_open())
			throw std::system_error(errno, std::system_category(), filename.c_str());
		size_t pos = filename.find('.');
		load(f, filename.substr(0, pos));
	}
#endif
	void load(std::istream& s, const std::string& army)
	{
		std::string line;
		while(std::getline(s, line))
		{
			if(line.length() < 2 || line[0] == '#')
				continue;
			Tokenizer t(line);
			std::string name = t.get();
			std::string type = t.get();
			if(name.length() && type.length())
			{
				Unit* u = UnitFactory::instance().make(name, type, army);
				m_units.push_back(std::unique_ptr<Unit>(u));
			}
		}
	}
	void run()
	{
		UserInterface& ui = UserInterface::instance();
		ui << "Welcome!" << std::endl << "故兵貴勝，不貴久。" << std::endl;;
		help();
		ui.prompt("Команда? > ");

		Command cmd;
		while(ui.good())
		{
			std::string s = ui.get_command();
			if(s.empty())
				continue;
			if(s == "?" || ieq(s, "help") || ieq(s, "wtf"))
			{
				help();
				continue;
			}
			if(ieq(s, "q") || ieq(s, "quit") || ieq(s, "exit") || ieq(s, "выход"))
				break;
			// assume regular command
			if(cmd.parse(s))
			{
				for(auto& u: m_units)
				{
					if(cmd.matches(*u))
						u->command(cmd);
				}
#ifdef HAVE_READLINE
				add_history(s.c_str());
#endif
			}
			else
				ui << "Неверная команда '" << s << "'" << std::endl;
		}

		ui << "Good bye!" << std::endl;
	}
	void dump(std::ostream& s) const
	{
		s << "Units:" << std::endl;
		for(auto &u: m_units)
		{
			u->dump(s);
			s << std::endl;
		}
	}
protected:
	void help()
	{
		UserInterface::instance() << "Общие команды:" << std::endl
			<< "'?' или 'help' - эта небольшая подсказка" << std::endl
			<< "'exit' или 'выход' или просто 'q' - он самый, выход" << std::endl
			<< std::endl;
		UserInterface::instance() << "Управление войсками:" << std::endl
			<< "<имя> <команда> - управление одной боевой единицей" << std::endl
			<< "все <команда> - скомандовать всем бевым единицам" << std::endl
			<< "армия <армия> <команда> - скомандовать всей армии <армия>" << std::endl
			<< "каждый <вид> <команда> - скомандовать всем боевым единицам вида <вид>" << std::endl
			<< "и <команда> - скомандовать тем же, что и в предыдущий раз" << std::endl
			<< std::endl;
	}
private:
	std::vector< std::unique_ptr<Unit> > m_units;
	std::set< std::string > m_types;
};

static void usage(const char* err = nullptr)
{
	std::ostream& s = err ? std::clog : std::cout;
	if(err)
		s << err << std::endl;
	s << "Usage:" << std::endl
		<< "\tgame -h\t--- this help" << std::endl
		<< "\tgame [-v] file1.army file2.army\t--- load army files" << std::endl
#if HAVE_BOOST_FILESYSTEM
		<< "\tgame [-v]\t--- load all *.army files in current directory" << std::endl
#endif
		<< std::endl;
}
int main(int argc, const char* argv[])
{
	bool verbose = false;
	/* poor man's getopt() */
	for(++argv; --argc; ++argv)
	{
		const char* p = argv[0];
		if(*p != '-')
			break;
		++p;
		while(*p)
		{
			switch(*p)
			{
			case 'h':
			case '?':
				usage();
				return 0;
			case 'v':
				verbose = true;
				break;
			default:
				usage("Bad option on command line");
				return 1;
			}
			++p;
		}
	}
	try
	{
		Game game;
		if(argc)
		{
			while(argc--)
				game.load(*argv++);
		}
		else
		{
#if HAVE_BOOST_FILESYSTEM
			using namespace boost::filesystem;
			directory_iterator it(".");
			directory_iterator end;
			for(; it != end; ++it)
			{
				if(! boost::filesystem::is_regular_file(it->status()))
					continue;
				std::string fn = it->path().filename().string();
				if(fn.length() < 5 || fn.substr(fn.length() - 5) != ".army")
					continue;
				game.load(it->path());
			}
#else
			usage("No armys to load");
			return 1;
#endif
		}
		if(verbose)
		{
			std::ostringstream ss;
			game.dump(ss);
			UserInterface::instance() << ss.str();
		}
		game.run();
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
		return -1;
	}
	return 0;
}


