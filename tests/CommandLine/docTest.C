// Demonstrates some of the command line basic documentation support.  This is a real world example, a combination of the man
// pages for "git" and "git-clone".  It concentrates mostly on the documentation, although the switches declarations are good
// enough that the command line can be parsed with them (they're just not as strict as possible, especially the "-c" switch).
//
// Some important notes:
//
//  (1) Never write hyphens as part of a switch name since not all operating systems use them.  Instead of "--help" and "-h"
//      write "@s{help}" and "@s{h}" and the documentation system will insert whatever is appropriate.  This even works if a
//      particular switch declaration or switch group overrides the prefix, and even works for switches whose names are not
//      recognized.
//
//  (2) When referring to another command, do it like this: "the @man{ls}(1) program lists files".  This will cause an
//      appropriate font to be used as well as a reference in the "see also" section.
//
//  (3) Use @v for variables, like "@v{name}".  Do not use upper-case names; the documentation will convert the name to
//      whatever format is needed for the backend.  If "@v{name}" is too verbose, you can also use the doxygen style "@v name"
//      (where "name" is everything to the next white space).  In fact, this works for all the tags, so you could have said
//      "the @man ls 1 program lists files" (@man is a bit unusual -- it takes two arguments, "ls" and "1"; incidentally,
//      saying "@man ls(1)" won't work because the first argument is everything to the next white space and there's no space
//      between the "ls" and the "(1)").  Besides curly braces and white space, you can use "()", "[]", and "<>".  In fact, the
//      thing inside the delimiters can use more of the same delimiters as long as they balance.  There cannot be white space
//      between the tag name and first argument and between arguments (unless, of course, white space is the delimiter).
//
//  (4) Use @em for emphasis
//
//  (5) To get a real "@em" into the document, add a backslash before it.  Since "@foobar" is not a recognized tag, there's no
//      need to add anything before it. Don't forget, this is C, so you'll need two backslashes.
//      FIXME[Robb Matzke 2014-02-24]: not tested yet
//
//  (6) Switch synopses may improve in the future.  For now, they just use the names you gave for arguments.
//
//  (7) The markup is meant to be readable yet precise.  I.e., it doesn't have the verbosity of XML or HTML, nor the ambiguity
//      of simple markup languages like Markdown.  The language has very few markup tags because it needs to support a variety
//      of backends.  We currently support ROFF for generating man pages (because it's installed on pretty much every unix
//      system in existence), but plan to also support PerlDoc, HTML, and plain text.  Perhaps even TeX.
//
//  (8) The API supports many other documentation related things that aren't demonstrated by this simple example.
//
// Run this like:
//      ./test11 --help

#include <Sawyer/CommandLine.h>

using namespace Sawyer;
using namespace Sawyer::CommandLine;

int main(int argc, char *argv[]) {

    SwitchGroup general;                                // general switches for all git commands
    general.insert(Switch("version")
                   .action(showVersion("1.0.0"))
                   .doc("Prints the git suite version that the git program came from."));
    general.insert(Switch("help")
                   .action(showHelp())
                   .doc("Prints the synopsis and a list of the most commonly used commands.  If the option @s all "
                        "or @s a is given then all available commands are printed.  If a git command is named, this "
                        "option will bring up the manual page for that command.\n\n"
                        "Other options are available to control how the manual page is displayed. See @man git-help 1 "
                        "for more information, because '@prop{programName} @s{help} ...' is converted internally into "
                        "'@prop{programName} help ...'"));
    general.insert(Switch("", 'c')
                   .argument("@v{name}=@v{value}")
                   .doc("Pass a configuration parameter to the command.  The value given will override values from "
                        "configuration files.  The @v name is expected in the same format as listed by @man git-config 1 "
                        "(subkeys separated by dots)."));
    general.insert(Switch("html-path")
                   .doc("Print the path, without trailing slash, where git's HTML documentation is installed and exit."));
    general.insert(Switch("man-path")
                   .doc("Print the manpath (see @man<man><1>) for the man pages for this version of git and exit."));
    general.insert(Switch("info-path")
                   .doc("Print the path where the Info files documenting this version of git are installed and exit."));
    general.insert(Switch("paginate", 'p')
                   .doc("Pipe all output into @man less 1 (or if set, $PAGER) if standard output is a terminal. This "
                        "overrides the pager.<cmd> configuration options (see the 'Configuration Mechanism' section "
                        "below)."));
    general.insert(Switch("no-pager")
                   .key("paginate")                             // same as the --paginate switch
                   .intrinsicValue("false", booleanParser<bool>())    // but with opposite value from the default
                   .doc("Do not pipe git output into a pager."));
    general.insert(Switch("git-dir")
                   .argument("path")
                   .doc("Set the path to the repository.  This can also be controlled by setting the GIT_DIR environment "
                        "variable.  It can be an absolute path or relative path to current working directory."));
    general.insert(Switch("namespace")
                   .argument("path")
                   .doc("Set the git namespace. See @man(gitnamespaces)(7) for more details.  Equivalent to setting the "
                        "GIT_NAMESPACE environment variable."));
    general.insert(Switch("bare")
                   .doc("Treat the repository as a bare repository.  If GIT_DIR environment is not set, it is set to the "
                        "current working directory."));
    general.insert(Switch("no-replace-objects")
                   .doc("Do not use replacement refs to replace git objects. See @man<git-replace>(1) for more information."));

    SwitchGroup cmd;                                    // switches particular to the git-clone command
    cmd.insert(Switch("local", 'l')
               .doc("When the repository to clone from is on a local machine, this flag bypasses the normal 'git aware' "
                    "transport mechanism and clones the repository by making a copy of HEAD and everything under objects "
                    "and refs directories. The files under .git/objects/ directory are hardlinked to save space when "
                    "possible. This is now the default when the source repository is specified with /path/to/repo syntax, "
                    "so it essentially is a no-op option. To force copying instead of hardlinking (which may be desirable "
                    "if you are trying to make a back-up of your repository), but still avoid the usual 'git aware' "
                    "transport mechanism, @s no-hardlinks can be used."));
    cmd.insert(Switch("no-hardlinks")
               .doc("Optimize the cloning process from a repository on a local filesystem by copying files under "
                    ".git/objects directory."));
    cmd.insert(Switch("shared", 's')
               .doc("When the repository to clone is on the local machine, instead of using hard links, automatically "
                    "setup .git/objects/info/alternates to share the objects with the source repository. The resulting "
                    "repository starts out without any object of its own.\n\n"
                    "@em NOTE: this is a possibly dangerous operation; do not use it unless you understand what it does. If "
                    "you clone your repository using this option and then delete branches (or use any other git command "
                    "that makes any existing commit unreferenced) in the source repository, some objects may become "
                    "unreferenced (or dangling). These objects may be removed by normal git operations (such as "
                    "'@prop{commandName} commit') which automatically call '@prop{commandName} gc @s auto'. "
                    " (See @man{git-gc}{1}.) If these objects are removed and were referenced by the cloned repository, then "
                    "the cloned repository will become corrupt.\n\n"
                    "Note that running '@prop{commandName} repack' without the @s{l} option in a repository cloned with "
                    "@s{s} will copy objects from the source repository into a pack in the cloned repository, removing the "
                    "disk space savings of clone @s{s}. It is safe, however, to run '@prop{commandName} gc', which uses the "
                    "@s{l} option by default.\n\n"
                    "If you want to break the dependency of a repository cloned with @s{s} on its source repository, you can "
                    "simply run '@prop{commandName} repack @s{a}' to copy all objects from the source repository into a "
                    "pack in the cloned repository."));
    cmd.insert(Switch("reference")
               .argument("repository")
               .doc("If the reference repository is on the local machine, automatically setup .git/objects/info/alternates "
                    "to obtain objects from the reference repository. Using an already existing repository as an alternate "
                    "will require fewer objects to be copied from the repository being cloned, reducing network and local "
                    "storage costs.\n\n"
                    "@em{@em NOTE}: see the NOTE for the @s shared option.")); // double emphasis
    cmd.insert(Switch("quiet", 'q')
               .doc("Operate quietly. Progress is not reported to the standard error stream. This flag is also passed to "
                    "the 'rsync' command when given."));
    cmd.insert(Switch("verbose", 'v')
               .doc("Run verbosely. Does not affect the reporting of progress status to the standard error stream."));
    cmd.insert(Switch("progress")
               .doc("Progress status is reported on the standard error stream by default when it is attached to a "
                    "terminal, unless @s{q} is specified. This flag forces progress status even if the standard error stream "
                    "is not directed to a terminal."));
    cmd.insert(Switch("no-checkout", 'n')
               .doc("No checkout of HEAD is performed after the clone is complete."));
    cmd.insert(Switch("bare")
               .doc("Make a bare GIT repository. That is, instead of creating @v directory and placing the administrative "
                    "files in @v{directory}/.git, make the @v directory itself the $GIT_DIR. This obviously implies the @s{n} "
                    "because there is nowhere to check out the working tree. Also the branch heads at the remote are "
                    "copied directly to corresponding local branch heads, without mapping them to refs/remotes/origin/. "
                    "When this option is used, neither remote-tracking branches nor the related configuration variables "
                    "are created."));
    cmd.insert(Switch("mirror")
               .doc("Set up a mirror of the source repository. This implies @s{bare}. Compared to @s{bare}, @s mirror not only "
                    "maps local branches of the source to local branches of the target, it maps all refs (including "
                    "remote-tracking branches, notes etc.) and sets up a refspec configuration such that all these refs "
                    "are overwritten by a '@prop{commandName} remote update' in the target repository."));
    cmd.insert(Switch("origin", 'b')
               .argument("name")
               .doc("Instead of pointing the newly created HEAD to the branch pointed to by the cloned repository's HEAD, "
                    "point to @v name branch instead.  @s branch can also take tags and treat them like detached HEAD. In a "
                    "non-bare repository, this is the branch that will be checked out."));
    cmd.insert(Switch("upload-pack", 'u')
               .argument("upload-pack")
               .doc("When given, and the repository to clone from is accessed via ssh, this specifies a non-default path "
                    "for the command run on the other end."));
    cmd.insert(Switch("template")
               .argument("template-directory")
               .doc("Specify the directory from which templates will be used; (See the 'TEMPLATE DIRECTORY' section of "
                    "@man{git-init}{1}.)"));
    cmd.insert(Switch("config", 'c')
               .argument("@v{key}=@v{value}")
               .doc("Set a configuration variable in the newly-created repository; this takes effect immediately after the "
                    "repository is initialized, but before the remote history is fetched or any files checked out. The key "
                    "is in the same format as expected by @man{git-config}{1} (e.g., core.eol=true). If multiple values are "
                    "given for the same key, each value will be written to the config file. This makes it safe, for "
                    "example, to add additional fetch refspecs to the origin remote."));
    cmd.insert(Switch("depth")
               .argument("depth", nonNegativeIntegerParser<unsigned>())
               .doc("Create a shallow clone with a history truncated to the specified number of revisions. A shallow "
                    "repository has a number of limitations (you cannot clone or fetch from it, nor push from nor into "
                    "it), but is adequate if you are only interested in the recent history of a large project with a long "
                    "history, and would want to send in fixes as patches."));
    cmd.insert(Switch("single-branch")
               .doc("Clone only the history leading to the tip of a single branch, either specified by the @s branch option "
                    "or the primary branch remote's HEAD points at. When creating a shallow clone with the @s depth option, "
                    "this is the default, unless @s no-single-branch is given to fetch the histories near the tips of all "
                    "branches."));
    cmd.insert(Switch("no-single-branch")
               .key("single-branch")
               .intrinsicValue("false", booleanParser<bool>())
               .hidden(true));                          // already documented in "single-branch"
    cmd.insert(Switch("recursive")
               .longName("recursive-submodules")        // alternate name for same switch
               .doc("After the clone is created, initialize all submodules within, using their default settings. This is "
                    "equivalent to running '@prop{commandName} submodule update @s init @s{recursive}' immediately after "
                    "the clone is "
                    "finished. This option is ignored if the cloned repository does not have a worktree/checkout (i.e. if "
                    "any of @s{no-checkout}/@s{n}, @s{bare}, or @s{mirror} is given)"));
    cmd.insert(Switch("separate-git-dir")
               .argument("git-dir")
               .doc("Instead of placing the cloned repository where it is supposed to be, place the cloned repository at "
                    "the specified directory, then make a filesytem-agnostic git symbolic link to there. The result is git "
                    "repository can be separated from working tree."));
    

    Parser parser;
    parser
        // These are the switch groups from above that we want to be able to parse.
        // FIXME[Robb Matzke 2014-02-26]: order matters, so document it
        .with(general)
        .with(cmd)

#if 0 /* enable these to turn this into a Windows-like command-parser, and notice how documentation also changes */
        .resetLongPrefixes("/")
        .resetShortPrefixes("/")
#endif

        // Basic information about the program. We could specify a program name here too, but Sawyer is smart
        // enough to get the name from the operating system, and that way your documentation will be relevant to
        // what the user actually typed to run the command.
        .chapter(1, "Sawyer Demonstration")                     // man page header and footer, i.e., "test11(1)"
        .purpose("Clone a repository into a new directory")     // the Name section for the benefit of makewhatis(1)
        .version("1.7.10.4", "09/29/2012")                      // appears in the manpage footer

        // The default synopsis is like "foo [SWITCHES...]". Capitalization is only for the sake of the output (but for nroff
        // it doesn't matter since it'll be upper-cased). Sawyer itself is not sensitive to how you capitalize the names; it
        // will treat "Synopsis" and "synopsis" the same way.
        .doc("Synopsis",
             "@prop{programName} clone [@v{switches}] @v{repository} @v{directory}")

        // An optional description gets output between the Synopsis and Options sections.  Of course, you don't have
        // do declare it in this order, but source code is more readable if you do.
        .doc("Description",
             "Clones a repository into a newly created directory, creates remote-tracking branches for each branch in "
             "the cloned repository (visible using '@prop{programName} branch @s{r}'), and creates and checks out an initial "
             "branch that is forked from the cloned repository's currently active branch.\n\n"
             "After the clone, a plain '@prop{programName} fetch' without arguments will update all the remote-tracking "
             "branches, and a '@prop{programName} pull' without arguments will in addition merge the remote master branch "
             "into the current master branch, if any.\n\n"
             "This default configuration is achieved by creating references to the remote branch heads under "
             "refs/remotes/origin and by initializing remote.origin.url and remote.origin.fetch configuration "
             "variables.\n\n"
             "This is version @prop{versionString}.")
        .doc("Options",
             "This paragraph is extra text for the Options section of the man page. It will appear above the list of "
             "program switches.  The common practice is to name this section 'Options' even though it mostly describes "
             "program switches. The remainder of this section is generated automatically.")

        // I always like to put the nitty gritty details after the command-line switches because I'm looking at a man
        // page because I forgot how to use a program, not because I forgot what it does.
        .doc("details", "1",
             "More details... Any number of user-defined sections are possible and their order can be controled.")

        // Extra stuff, some standard and some funny. Sawyer doesn't care what sections you define.
        .doc("Bugs", "2", "User-defined sections can be in any order you want.")
        .doc("More Bugs", "3", "...and named anything you want.")

        // The See Also section is automatically generated and contains all the @man references that appear prior to
        // this section (that is, prior in the output, not the C++ source code).  You can also insert your own text.
        .doc("See Also", "zzz", "And you can augment sections that are created automatically.")

        // The markup parser is decently smart about deciding when a "@" is part of a tag and when it should be copied
        // to the output.  For instance, the "@" in the email address works fine.  Also, the two @man references here show
        // up in the See Also section since "authors" comes before "zzz" alphabetically.
        .doc("Authors",
             "Git was started by Linus Torvalds, and is currently maintained by Junio C Hamano. Numerous contributions "
             "have come from the git mailing list @link{git@vger.kernel.org}{}. For a more complete list of contributors, see "
             "@link{http://git-scm.com/about}{}. If you have a clone of git.git itself, the output of @man{git-shortlog}(1) "
             " and @man{git-blame}(1) can show you the authors for specific parts of the projecyt.")

        // You can delete some sections (this example's a bit stupid because you wouldn't normally delete a section you
        // just created, but you might want to delete a section if the parser was created in source code you don't "own".
        // You can't delete the sections Sawyer creates automatically (you'd only delete your own text in those sections).
        .doc("Bogus", "Something we're about to delete...")
        .doc("BOGUS", "") // case is insignificant

        // Normally if an error occurs Sawyer will throw an STL runtime_error containing the error message.  But if you set
        // the errorStream property to a Sawyer::Message::Stream your error will go there instead and the program exits with
        // non-zero status.  If you prefer, you can catch the runtime_error and print it yourself, or even just rely on the C++
        // runtime to print it.  Also, if an errorStream is used and a "help" switch is defined, Sawyer will print "invoke
        // with '--help' for usage information." (or you can customize it to something else with the exitMessage property.
        .errorStream(Message::mlog[Message::FATAL]) // exit on error instead of throwing std::runtime_error

        // Finally, parse the command line!  This returns a ParserResult which we don't use in this example.
        .parse(argc, argv)

        // Cause actions to run and values to be saved.
        .apply();
}
