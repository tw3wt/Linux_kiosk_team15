#define NAMESIZE 20
#define PRICSIZE 3
#define START_ID 1
#define MAX_ITEM 9999

#define DESSERT	"dessert"
#define DRINK	"drink"
#define SALES	"sales"

//size 32
struct dessert{
	char name[NAMESIZE];
	int	id;
	int price;
	int number;
};

//size 36
struct drink{
	char name[NAMESIZE];
	int id;
	int price[PRICSIZE];
};

struct sales{ //5.31 tae hyeon
	int index;
	int total_sales;
	time_t sale_time;
};
