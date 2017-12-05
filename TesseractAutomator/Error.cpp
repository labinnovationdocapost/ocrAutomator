#include "Error.h"
#include "Display.h"
#include "SlaveDisplay.h"
#include <cstdio>
#include <execinfo.h>

extern Display* display;
extern SlaveDisplay* sdisplay;
extern std::thread* th;

#if DISPLAY
void segfault_action(int sig, siginfo_t *info, void *secret)
{
	if (display != nullptr)
		display->terminated(true);
	if (sdisplay != nullptr)
		delete sdisplay;
	void *trace[16];
	char **messages = (char **)NULL;
	int i, trace_size = 0;
	ucontext_t *uc = (ucontext_t *)secret;

	auto file = fopen("/var/log/TesseractAutomatorLog.log", "w");
	/* Do something useful with siginfo_t */
	if (sig == SIGSEGV)
		fprintf(file, "Got signal %d, faulty address is %p, "
			"from %p\n", sig, info->si_addr,
			uc->uc_mcontext.gregs[REG_RIP]);
	else
		fprintf(file, "Got signal %d\n", sig);

	std::cout << "Segfault happened view /var/log/TesseractAutomatorLog.log for more details" << std::endl << std::flush;

	trace_size = backtrace(trace, 16);
	/* overwrite sigaction with caller's address */
	trace[1] = (void *)uc->uc_mcontext.gregs[REG_RIP];

	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	fprintf(file, "[bt] Execution path:\n");
	for (i = 1; i < trace_size; ++i)
	{
		fprintf(file, "[bt] %s\n", messages[i]);

		/* find first occurence of '(' or ' ' in message[i] and assume
		* everything before that is the file name. (Don't go beyond 0 though
		* (string terminator)*/
		size_t p = 0;
		while (messages[i][p] != '(' && messages[i][p] != ' '
			&& messages[i][p] != 0)
			++p;

		char syscom[512];
		sprintf(syscom, "addr2line %p -e %.*s", trace[i], p, messages[i]);
		//last parameter is the filename of the symbol
		FILE* ptr;
		char buf[BUFSIZ];

		fprintf(file, "%s\n", syscom);
		if ((ptr = popen(syscom, "r")) != NULL) {
			while (fgets(buf, BUFSIZ, ptr) != NULL)
			{

				fprintf(file, "%s", buf);
			}
			pclose(ptr);
		}
	}
	if (th->joinable())
		th->join();
	exit(0);
}
void CatchAllErrorSignals()
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = segfault_action;
	sa.sa_flags = SA_SIGINFO | SA_RESTART;

	sigaction(SIGSEGV, &sa, nullptr);
}
void CatchAllExceptions()
{
	std::set_terminate([]()
	{
		if (display != nullptr)
			display->terminated(true);
		if (sdisplay != nullptr)
			delete sdisplay;


		auto eptr = std::current_exception();
		auto n = eptr.__cxa_exception_type()->name();
		std::cout << "Exception " << n << " happened view /var/log/TesseractAutomatorLog.log for more details" << std::endl << std::flush;
		std::cerr << "Unhandled exception " << n << std::endl;

		void *trace_elems[20];
		int trace_elem_count(backtrace(trace_elems, 20));
		char **stack_syms(backtrace_symbols(trace_elems, trace_elem_count));
		for (int i = 0; i < trace_elem_count; ++i)
		{
			std::cerr << stack_syms[i] << std::endl;
		}
		free(stack_syms);

		std::abort();
	});
}

void GenerateSIGSEGV()
{
	*(int*)0 = 0;
}
#endif
