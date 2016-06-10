#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <memory>
#include <set>
#include <sstream>
#ifdef HAVE_READLINE
# include <readline/readline.h>
# include <readline/history.h>
#endif

/*
 *
 * Тут "чистый" C++11, никакого boostа. Причина тому - отсутствие опыта
 * (привычки) и отсутствие необходимости. Возможно, в следующей версии,
 * попробую переделать части на boost.
 *
 * Для запуска на Windows нужно использовать консоль с кодировкаой UTF-8
 * (например, Git bash here из последнего Git for windows) либо перед
 * сборкой перекодировать все файлы в CP866.
 *
 * Куда можно двигаться:
 *  - сделать вывод через объект UserInterface
 *  - растащить исходник на несколько файлов .h и .cxx
 *  - написать документацию к классам
 *  - сделать алиасы командам
 *  - сделать команды и названия регистронезависимыми (locale?)
 *  - переделать токенизатор команды (boost)
 *  - сделать загрузку всех файлов армий из каталога (boost filesystem?)
 * Куда можно двигаться дальше:
 *  - приделать карту и координаты каждому юниту
 *  - дать войскам оружие
 *  - приделать противника
 *  - приделать противнику AI
 *  - приделать network multiplayer :)
 *
 */

template<class T>
struct StringMatchHelper
{
	bool (T::*method)();
	const char* cmd;
	inline bool matches(const std::string& s) const
	{
		return s == cmd;
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
	void report(const char* phrase)
	{
		std::cout << name() << ": " << phrase << std::endl;
	}
	virtual void dump(std::ostream& s) const
	{
		s << "Unit: " << m_name << " (type: " << m_type << ", army: " << m_army << ")";
	}
private:
	bool ping()
	{
		report("Still alive");
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
#define MK(nm, cls) if(type == nm) return new cls(name, type, army);
		MK("пехотинец", WalkingUnit);
		MK("танк", WalkingUnit);
		MK("вертолёт", FlyingUnit);
#undef MK
		throw std::runtime_error("Unknown unit type");
	}
};

class UserInterface
{
private:
	UserInterface()
		: m_good(true)
	{
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
#else
		std::cout << m_prompt;
		std::cout.flush();
		std::getline(std::cin, r);
		m_good = std::cin.good();
#endif
		return r;
	}
private:
	bool m_good;
	std::string m_prompt;
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
		if(cmd[0] == "и" && cmd.size() == 2)
		{
			m_cmd = cmd[1];
			return true;
		}
		else
			m_name = m_type = m_army = m_cmd = "";
		if(cmd.size() == 2)
		{
			if(cmd[0] != "все")
				m_name = cmd[0];
			m_cmd = cmd[1];
			return true;
		}
		if(cmd.size() == 3 && cmd[0] == "армия")
		{
			m_army = cmd[1];
			m_cmd = cmd[2];
			return true;
		}
		if(cmd.size() == 3 && cmd[0] == "каждый")
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
	void load(const std::string& filename) /* XXX TODO: check unicode filenames on win32 XXX */
	{
		std::ifstream f(filename.c_str());
		size_t pos = filename.find('.');
		load(f, filename.substr(0, pos));
	}
	void load(std::istream& s, const std::string& army)
	{
		while(s.good())
		{
			std::string line;
			std::getline(s, line);
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
		std::cout << "Welcome!" << std::endl;
		help();
		ui.prompt("Command? > ");

		Command cmd;
		while(ui.good())
		{
			std::string s = ui.get_command();
			if(s.empty())
				continue;
			if(s == "?")
			{
				help();
				continue;
			}
			if(s == "q" || s == "quit" || s == "exit" || s == "выход")
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
				std::cerr << "Bad command '" << s << "'" << std::endl;
		}

		std::cout << "Good bye!" << std::endl;
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
		std::cout << "General commands:" << std::endl
			<< "? for help" << std::endl
			<< "exit to leave this excellent game" << std::endl
			<< std::endl;
		std::cout << "Unit controlling commands:" << std::endl
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
	s << "Usage:\n\tgame -h\t--- this help\n\tgame file1.army file2.army\t--- load army files\n\n";
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
			usage("No armys to load");
			return 1;
		}
		if(verbose)
			game.dump(std::cout);
		game.run();
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
		return -1;
	}
	return 0;
}


