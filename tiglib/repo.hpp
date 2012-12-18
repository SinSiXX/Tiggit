#ifndef __TIGLIB_REPO_HPP_
#define __TIGLIB_REPO_HPP_

#include <stdint.h>
#include <spread/job/jobinfo.hpp>
#include <spread/misc/jconfig.hpp>
#include <boost/shared_ptr.hpp>
#include "list/mainlist.hpp"
#include "gamedata.hpp"

namespace Spread { struct SpreadLib; }

namespace TigLib
{
  class Repo
  {
    struct _Internal;
    boost::shared_ptr<_Internal> ptr;

    std::string dir;
    std::string tigFile, statsFile, newsFile, shotDir, spreadDir;
    Misc::JConfig conf, inst;
    int64_t lastTime;

    void setDirs();

  public:
    Repo(bool runOffline=false)
      : offline(runOffline) {}

    // Set to true to run in offline mode. You can switch this on/off
    // at any time. Various internal functions will skip trying to
    // connect to the net if this is set.
    bool offline;

    Misc::JConfig news, rates;

    // Check if the repository is locked. If not, we are not allowed
    // to write to it.
    bool isLocked() const;

    /* Find or establish a repository in the given location. An empty
       path means we should use the standard path for this OS. This
       will also look for repositories in legacy locations from older
       versions of Tiggit.

       Returns true on success, false if the repository is not usable
       (eg. if the path is not writable), or if no standard path was
       found.

       If it fails, you may prompt the user for a new location and try
       again. Optionally you may use defaultPath() to get a standard
       location suggestion.
     */
    bool findRepo(const std::string &where = "");

    /* Returns the default path to suggest to the user if no existing
       repository is found.
     */
    static std::string defaultPath();

    /* Return the location of any legacy repository location we could
       find. Returns "" if no legacy repo exists.

       A legacy repository is a repository made by an earlier and now
       outdated version of Tiggit. It cannot be used directly, but can
       be imported into the current repository using importFrom().
     */
    static std::string findLegacyDir();

    /* Set the repository directory directly. This is an alternative
       to findRepo() that doesn't use or update the global stored
       config settings.

       You can use this eg. to maintain several independent
       repositories, or to use a repository on a flash drive without
       disturbing the configuration of an existing installed
       repository.
     */
    void setRepo(const std::string &where);

    /* Initialize repository. This includes creating a lock file and
       loading configuration. The function will also convert legacy
       data from older versions of Tiggit. Call this after calling
       findRepo() or setRepo() successfully.

       Returns true on success, false if the locking failed.

       A 'false' return value usually either means that another
       process is using the repository, or that a process crashed
       while the repository was locked. The best course of action is
       to ask the user if they want to override the lock (call again
       with forceLock=true), and at the same time warn them about the
       possible consequences of this.

       Note that a forced lock may still fail under some
       circumstances.
     */
    bool initRepo(bool forceLock=false);

    /* Update all disk files from the net. May in some cases return a
       JobInfo pointer. If it does, then it is not safe to proceed
       (calling loadData) until the job has finished. Preferably you
       should inform the user about the download progress.

       If the pointer is empty or the info has isSuccess() set, you
       can continue loading immediately. If the info has isError() or
       isAbort() set, you may still be able to load the data, but
       errors may occur or the data may be outdated.

       May only be called on an initialized repository (initRepo()
       returned true.)
     */
    Spread::JobInfoPtr fetchFiles(bool includeShots=true, bool async=true);

    /* Returns true if the latest fetch updated the data set. Only
       valid after the fetchFiles() job has successfully completed.
     */
    bool hasNewData() const;

    /* Load current game data from the repository files into
       memory. Will clear out any existing data.
     */
    void loadData();

    /* Call this to propagate newly loaded data into the rest of the
       system. If you have used baseList() to build implicit
       dependencies on the list data, and these again depend on some
       data that has to be set up after loadData() is called (such as
       setting the LiveInfo::extra member pointers), then you will
       probably want to wait to call this until you have finished
       setting up.
    */
    void doneLoading();

    /* Load statistics data (download counts etc). This is
       automatically invoked by loadData(), but may also be invoked
       separately if you at some point update the stats file on disk
       and want to refresh the numbers in memory.

       The function ignores all errors and does not throw.
     */
    void loadStats();

    // Get the path of a file or directory within the repository. Only
    // valid after findRepo() has been invoked successfully. Use
    // without parameter to get the repo directory itself.
    std::string getPath(const std::string &fname = "") const;

    std::string getNewsFile() const { return newsFile; }

    // Fetch file from URL to the given location within the
    // repository. Returns the full path.
    std::string fetchPath(const std::string &url,
                          const std::string &fname);

    // Returns idnames of all games that are or have been installed.
    // Use getGameDir() to check actual install status of the game.
    std::vector<std::string> getGameList() { return inst.getNames(); }

    // Get actuall install dir for a game. Returns "" if the game is
    // not registered as installed.
    std::string getGameDir(const std::string &idname);

    // Get default install dir for a game
    std::string getDefGameDir(const std::string &idname) const
    { return getPath("gamedata/" + idname); }

    // Get screenshot path for a game.
    std::string getScreenshot(const std::string &idname) const;

    // Start installing a game
    Spread::JobInfoPtr startInstall(const std::string &idname,
                                    const std::string &urlname,
                                    std::string where,
                                    bool async=true); 

    // Start uninstalling a game
    Spread::JobInfoPtr startUninstall(const std::string &idname, bool async=true);
   
    // Remove any path, using boost::filesystem::remove_all(), in a
    // background thread.
    static Spread::JobInfoPtr killPath(const std::string &dir, bool async=true);

    // Get access to the Spread repository instance used internally by
    // Repo
    Spread::SpreadLib &getSpread() const;

    // Main lookup list of all games
    const InfoLookup &getList() const;

    // Use this to derive other ListBase structs from the main list.
    List::ListBase &baseList();

    // The last known TigEntry::addDate of our previous run.
    int64_t getLastTime() const { return lastTime; }

    // Get rating for a given game. Returns -1 if no rating is set.
    int getRating(const std::string &id);

    // Set rating, in range 0-5
    void setRating(const std::string &id, const std::string &urlname,
                   int rate);

    // Set new lastTime. Does NOT change the current lastTime field,
    // but instead stores the value in conf for our next run.
    void setLastTime(int64_t val);
  };
}
#endif
