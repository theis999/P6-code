using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Mvc.RazorPages;
using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Authorization;
using System.Net.Http;
using System.Security.Claims;
using System;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Diagnostics;
using System.Net.Http.Headers;

using System.Collections;

namespace newnewWebinterface.Pages;

public class game
{
    long id;
    string gamestate;
    string gamestart;
};


[Authorize]
public class IndexModel : PageModel
{

    [BindProperty]
    public IEnumerable<game> Games { get; set; }

    public string? AccessToken { get; set; }

    public async IAsyncEnumerable<game> OnGetAsync()
    {
        string authentikUserID = User.FindFirstValue("sub");

        var client = new HttpClient();
        AccessToken = await HttpContext.GetTokenAsync("access_token");
        client.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Bearer", AccessToken);

        using HttpResponseMessage response = await client.GetAsync("https://smakdb.head9x.dk/user/get/" + authentikUserID);
        // Debug
        System.Diagnostics.Debug.WriteLine("Token: " + AccessToken);
        System.Diagnostics.Debug.WriteLine("Response: " + response);
        Games = JsonSerializer.Deserialize<game>(response.Content);
        return Games.ToAsyncEnumerable();
    }
}
