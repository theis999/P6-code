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
using System.Linq;

using System.Collections;

namespace newnewWebinterface.Pages;

[Authorize]

public class IndexModel : PageModel
{

    [BindProperty]
    public IEnumerable<Game>? Games { get; set; }

    [BindProperty]
    public IEnumerable<Board>? Boards { get; set; }

    public string? AccessToken { get; set; }



    public class Game
    {
        public long? id { get; set; }
        string? gamestate { get; set; }
        string? gamestart { get; set; }
    };

    public class Board
    {
        public long? id { get; set; }
        string? product_id { get; set; }
        public string? name { get; set; }
        public IEnumerable<Game>? games { get; set; }
    };

    public async Task<PageResult> OnGetAsync()
    {
        string authentikUserID = User.FindFirstValue("sub");

       /// var client = new HttpClient();
        AccessToken = await HttpContext.GetTokenAsync("access_token");
       /// client.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Bearer", AccessToken);

       /// using HttpResponseMessage response = await client.GetAsync("https://smakdb.head9x.dk/user/get/" + authentikUserID);

        var boardclient = new HttpClient();
        boardclient.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Bearer", AccessToken);

        using HttpResponseMessage boardresponse = await boardclient.GetAsync("https://smakdb.head9x.dk/boards/get/" + authentikUserID);
        // Debug
        //System.Diagnostics.Debug.WriteLine("Token: " + AccessToken);
        //System.Diagnostics.Debug.WriteLine("Response: " + response);
        /// Games = await response.Content.ReadFromJsonAsync<IEnumerable<Game>>();
        if (!boardresponse.IsSuccessStatusCode)
        {
            Boards = new List<Board>
            {
                new Board
                {
                    id = -1,
                    name = "No Boards found",
                    games = Enumerable.Empty<Game>()
                }
            };

            return Page();
        }
        Boards = await boardresponse.Content.ReadFromJsonAsync<IEnumerable<Board>>();

        if (Boards == null || !Boards.Any())
        {
            Boards = new List<Board>
            {
                new Board
                {
                    id = -1,
                    name = "No Boards found",
                    games = Enumerable.Empty<Game>()
                }
            };
        }

        return Page();
    }
}
